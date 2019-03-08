#pragma once

#include "common/Singleton.hpp"

#define GIF_FRAME_LENGTH 33

#include "providers/bttv/BttvEmotes.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "singletons/helper/GifTimer.hpp"

namespace chatterino
{
    class Settings;
    class Paths;

    class Emotes final : public Singleton
    {
    public:
        Emotes();

        virtual void initialize(Settings& settings, Paths& paths) override;

        bool isIgnoredEmote(const QString& emote);

        GIFTimer gifTimer;
    };
}  // namespace chatterino
