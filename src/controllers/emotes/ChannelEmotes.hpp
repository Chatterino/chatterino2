#pragma once

#include "common/Aliases.hpp"

#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <memory>
#include <span>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteProvider;
class EmoteMap;
using EmoteMapPtr = std::shared_ptr<const EmoteMap>;
class EmoteController;

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class TwitchChannel;

/// Emote manager for one channel.
///
/// Conceptually, this is a managed list of "instantiated" providers for a
/// channel. It stores the emotes for each provider and takes care of
/// initializing and reloading them. Providers are sorted by their priority.
class ChannelEmotes
{
public:
    /// An entry for a single provider
    struct ProviderData {
        std::weak_ptr<EmoteProvider> provider;
        QString id;
        EmoteMapPtr emotes;
    };

    ChannelEmotes(TwitchChannel *channel);
    ChannelEmotes(const ChannelEmotes &) = delete;
    ChannelEmotes(ChannelEmotes &&) = delete;
    ChannelEmotes &operator=(const ChannelEmotes &) = delete;
    ChannelEmotes &operator=(ChannelEmotes &&) = delete;
    ~ChannelEmotes() = default;

    void initialize(const EmoteController &controller);

    /// Get an emote by its name.
    ///
    /// If there's no emote, an empty shared_ptr is returned.
    EmotePtr resolve(const EmoteName &query) const;

    /// Refresh all channel emotes
    ///
    /// @param manualRefresh Was this triggered by a user-action. If so, a
    ///                      message will always be added to the channel.
    void refresh(bool manualRefresh);

    ProviderData *dataForProvider(const std::weak_ptr<EmoteProvider> &provider);
    std::span<const ProviderData> providerData() const
    {
        return this->providerData_;
    }

private:
    static void refreshProvider(const ProviderData &data,
                                TwitchChannel *channel, bool manualRefresh);

    void sort();

    std::vector<ProviderData> providerData_;

    TwitchChannel *channel;
    pajlada::Signals::SignalHolder signalHolder;
};

}  // namespace chatterino
