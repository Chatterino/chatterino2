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
class EmoteChannel;

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

/// An entry for a single provider
struct EmoteHolderItem {
    std::weak_ptr<EmoteProvider> provider;
    QString id;
    EmoteMapPtr emotes;
};

class EmoteHolder
{
public:
    using Item = EmoteHolderItem;

    EmoteHolder(EmoteChannel *Channel);

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

    Item *itemByProvider(const std::weak_ptr<EmoteProvider> &provider);
    std::span<const Item> items() const
    {
        return this->items_;
    }

private:
    static void refreshItem(const Item &item, EmoteChannel *channel,
                            bool manualRefresh);

    void sort();

    std::vector<Item> items_;

    EmoteChannel *channel;
    pajlada::Signals::SignalHolder signalHolder;
};

}  // namespace chatterino
