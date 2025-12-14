#include "controllers/emotes/EmoteProvider.hpp"

#include "controllers/emotes/ChannelEmotes.hpp"

namespace chatterino {

EmoteProvider::EmoteProvider(QString name, QString id, uint32_t priority)
    : globalEmotes_(EMPTY_EMOTE_MAP)
    , name_(std::move(name))
    , id_(std::move(id))
    , priority_(priority)
{
}

EmoteProvider::~EmoteProvider() = default;

EmoteMapPtr EmoteProvider::globalEmotes() const
{
    return this->globalEmotes_;
}

EmotePtr EmoteProvider::globalEmote(const EmoteName &name) const
{
    auto it = this->globalEmotes_->find(name);
    if (it == this->globalEmotes_->end())
    {
        return nullptr;
    }
    return it->second;
}

EmotePtr EmoteProvider::createEmote(Emote &&emote)
{
    auto &slot = cache[emote.id];
    auto shared = slot.lock();
    if (shared && *shared == emote)
    {
        // reuse old shared_ptr if nothing changed
        return shared;
    }

    shared = std::make_shared<Emote>(std::move(emote));
    slot = shared;
    return shared;
}

}  // namespace chatterino
