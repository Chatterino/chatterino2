#pragma once

#include <pajlada/signals/signal.hpp>
#include <QString>

#include <memory>

namespace chatterino {

namespace liveupdates {
struct Diag;
}  // namespace liveupdates

struct BttvLiveUpdateEmoteUpdateAddMessage;
struct BttvLiveUpdateEmoteRemoveMessage;

class BttvLiveUpdatesPrivate;
class BttvLiveUpdates
{
    template <typename T>
    using Signal = pajlada::Signals::Signal<T>;

public:
    BttvLiveUpdates(QString host);
    ~BttvLiveUpdates();

    struct {
        Signal<BttvLiveUpdateEmoteUpdateAddMessage> emoteAdded;
        Signal<BttvLiveUpdateEmoteUpdateAddMessage> emoteUpdated;
        Signal<BttvLiveUpdateEmoteRemoveMessage> emoteRemoved;
    } signals_;  // NOLINT(readability-identifier-naming)

    /**
     * Joins a Twitch channel by its id (without any prefix like 'twitch:')
     * if it's not already joined.
     *
     * @param channelID the Twitch channel-id of the broadcaster.
     * @param userID the Twitch user-id of the current user.
     */
    void joinChannel(const QString &channelID, const QString &userID);

    void broadcastMe(const QString &channelID, const QString &userID);

    /**
     * Parts a twitch channel by its id (without any prefix like 'twitch:')
     * if it's joined.
     *
     * @param id the Twitch channel-id of the broadcaster.
     */
    void partChannel(const QString &id);

    /// Stop the manager
    ///
    /// Used in tests to check that connections are closed (through #diag()).
    /// Otherwise, calling the destructor is sufficient.
    void stop();

    /// Statistics about the opened/closed connections and received messages
    ///
    /// Used in tests.
    const liveupdates::Diag &diag() const;

private:
    std::unique_ptr<BttvLiveUpdatesPrivate> private_;
};

}  // namespace chatterino
