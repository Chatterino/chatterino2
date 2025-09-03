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

    EmoteProviderPtr findProviderByName(QStringView name) const;
    EmoteProviderPtr findProviderByID(QStringView id) const;

    EmotePtr resolveGlobal(const EmoteName &query) const;

    std::span<const EmoteProviderPtr> providers() const;

    TwitchEmotes *twitchEmotes() const;

    Emojis *emojis() const;

    GIFTimer *gifTimer() const;

protected:
    void sort();

    std::vector<EmoteProviderPtr> providers_;

private:
    std::unique_ptr<TwitchEmotes> twitchEmotes_;
    std::unique_ptr<Emojis> emojis_;
    std::unique_ptr<GIFTimer> gifTimer_;
};

}  // namespace chatterino
