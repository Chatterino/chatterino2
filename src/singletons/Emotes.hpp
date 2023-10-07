#pragma once

#include "common/Singleton.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "singletons/helper/GifTimer.hpp"

namespace chatterino {

class Settings;
class Paths;

class IEmotes
{
public:
    virtual ~IEmotes() = default;

    virtual ITwitchEmotes *getTwitchEmotes() = 0;
    virtual IEmojis *getEmojis() = 0;
};

class Emotes final : public IEmotes, public Singleton
{
public:
    Emotes();

    virtual void initialize(Settings &settings, Paths &paths) override;

    bool isIgnoredEmote(const QString &emote);

    ITwitchEmotes *getTwitchEmotes() final
    {
        return &this->twitch;
    }

    IEmojis *getEmojis() final
    {
        return &this->emojis;
    }

    TwitchEmotes twitch;
    Emojis emojis;

    GIFTimer gifTimer;
};

}  // namespace chatterino
