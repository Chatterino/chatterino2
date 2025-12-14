#include "controllers/emotes/EmoteController.hpp"

#include "controllers/emotes/EmoteProvider.hpp"
#include "providers/emoji/Emojis.hpp"
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

    this->sort();

    for (const auto &provider : this->providers_)
    {
        provider->initialize();
    }
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

std::span<const EmoteProviderPtr> EmoteController::getProviders() const
{
    return this->providers_;
}

TwitchEmotes *EmoteController::getTwitchEmotes() const
{
    return this->twitchEmotes_.get();
}

Emojis *EmoteController::getEmojis() const
{
    return this->emojis_.get();
}

GIFTimer *EmoteController::getGIFTimer() const
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
