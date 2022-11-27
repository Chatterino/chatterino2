#pragma once

#include "common/Singleton.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
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

    TwitchEmotes twitch;
    Emojis emojis;

    GIFTimer gifTimer;
};

}  // namespace chatterino
