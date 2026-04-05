// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/signals/signal.hpp>
#include <QString>

#include <memory>

namespace chatterino {

namespace liveupdates {
struct Diag;
}  // namespace liveupdates

namespace seventv::eventapi {
struct EmoteAddDispatch;
struct EmoteUpdateDispatch;
struct EmoteRemoveDispatch;
struct UserConnectionUpdateDispatch;
}  // namespace seventv::eventapi

class SeventvBadges;

class SeventvEventAPIPrivate;
class SeventvEventAPI
{
    template <typename T>
    using Signal = pajlada::Signals::Signal<T>;

public:
    SeventvEventAPI(QString host,
                    std::chrono::milliseconds defaultHeartbeatInterval =
                        std::chrono::milliseconds(25000));
    ~SeventvEventAPI();

    struct {
        Signal<seventv::eventapi::EmoteAddDispatch> emoteAdded;
        Signal<seventv::eventapi::EmoteUpdateDispatch> emoteUpdated;
        Signal<seventv::eventapi::EmoteRemoveDispatch> emoteRemoved;
        Signal<seventv::eventapi::UserConnectionUpdateDispatch> userUpdated;
    } signals_;  // NOLINT(readability-identifier-naming)

    /**
     * Subscribes to a user and emote-set
     * if not already subscribed.
     *
     * @param userID 7TV user-id, may be empty.
     * @param emoteSetID 7TV emote-set-id, may be empty.
     */
    void subscribeUser(const QString &userID, const QString &emoteSetID);
    /**
     * Subscribes to cosmetics and entitlements in a Twitch channel
     * if not already subscribed.
     *
     * @param id Twitch channel id
     */
    void subscribeTwitchChannel(const QString &id);

    /** Unsubscribes from a user by its 7TV user id */
    void unsubscribeUser(const QString &id);
    /** Unsubscribes from an emote-set by its id */
    void unsubscribeEmoteSet(const QString &id);
    /** Unsubscribes from cosmetics and entitlements in a Twitch channel */
    void unsubscribeTwitchChannel(const QString &id);

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
    std::unique_ptr<SeventvEventAPIPrivate> private_;
};

}  // namespace chatterino
