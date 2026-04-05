// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

namespace chatterino {

class TwitchEmotes;
class Emojis;
class GIFTimer;

class EmoteController
{
public:
    EmoteController();
    virtual ~EmoteController();

    virtual void initialize();

    TwitchEmotes *getTwitchEmotes() const;

    Emojis *getEmojis() const;

    GIFTimer *getGIFTimer() const;

private:
    std::unique_ptr<TwitchEmotes> twitchEmotes_;
    std::unique_ptr<Emojis> emojis_;
    std::unique_ptr<GIFTimer> gifTimer_;
};

}  // namespace chatterino
