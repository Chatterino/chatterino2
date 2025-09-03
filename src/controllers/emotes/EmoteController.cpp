#include "controllers/emotes/EmoteController.hpp"

#include "providers/bttv/BttvEmoteProvider.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/ffz/FfzEmoteProvider.hpp"
#include "providers/seventv/SeventvEmoteProvider.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "singletons/helper/GifTimer.hpp"

namespace chatterino {

EmoteController::EmoteController()
    : twitchEmotes_(std::make_unique<TwitchEmotes>())
    , emojis_(std::make_unique<Emojis>())
    , gifTimer_(std::make_unique<GIFTimer>())
{
}
EmoteController::~EmoteController() = default;

void EmoteController::initialize()
{
    this->emojis_->load();
    this->gifTimer_->initialize();

    this->providers_.emplace_back(std::make_shared<BttvEmoteProvider>());
    this->providers_.emplace_back(std::make_shared<FfzEmoteProvider>());
    this->providers_.emplace_back(std::make_shared<SeventvEmoteProvider>());
    this->sort();

    for (const auto &provider : this->providers_)
    {
        provider->initialize();
    }
}

EmoteProviderPtr EmoteController::findProviderByName(QStringView name) const
{
    auto it = std::ranges::find_if(this->providers_, [&](const auto &provider) {
        return provider->name() == name;
    });
    if (it == this->providers_.end())
    {
        return nullptr;
    }
    return *it;
}

EmoteProviderPtr EmoteController::findProviderByID(QStringView id) const
{
    auto it = std::ranges::find_if(this->providers_, [&](const auto &provider) {
        return provider->id() == id;
    });
    if (it == this->providers_.end())
    {
        return nullptr;
    }
    return *it;
}

EmotePtr EmoteController::resolveGlobal(const EmoteName &query) const
{
    for (const auto &provider : this->providers_)
    {
        auto emote = provider->globalEmote(query);
        if (emote)
        {
            return emote;
        }
    }
    return nullptr;
}

std::span<const EmoteProviderPtr> EmoteController::providers() const
{
    return this->providers_;
}

TwitchEmotes *EmoteController::twitchEmotes() const
{
    return this->twitchEmotes_.get();
}

Emojis *EmoteController::emojis() const
{
    return this->emojis_.get();
}

GIFTimer *EmoteController::gifTimer() const
{
    return this->gifTimer_.get();
}

void EmoteController::sort()
{
    std::ranges::sort(this->providers_, {}, [](const auto &provider) {
        return provider->priority();
    });
}

}  // namespace chatterino
