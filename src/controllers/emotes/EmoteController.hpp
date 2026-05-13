// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/Emote.hpp"

#include <pajlada/signals/signal.hpp>

#include <memory>
#include <span>
#include <vector>

namespace chatterino {

class EmoteProvider;
using EmoteProviderPtr = std::shared_ptr<EmoteProvider>;

class TwitchEmotes;
class Emojis;
class GIFTimer;

class EmoteController
{
public:
    EmoteController();
    virtual ~EmoteController();

    virtual void initialize();

    EmotePtr resolveGlobal(const EmoteName &query) const;

    std::span<const EmoteProviderPtr> getProviders() const;

    TwitchEmotes *getTwitchEmotes() const;

    Emojis *getEmojis() const;

    GIFTimer *getGIFTimer() const;

protected:
    void sort();

    std::vector<EmoteProviderPtr> providers_;

private:
    std::unique_ptr<TwitchEmotes> twitchEmotes_;
    std::unique_ptr<Emojis> emojis_;
    std::unique_ptr<GIFTimer> gifTimer_;
};

}  // namespace chatterino
