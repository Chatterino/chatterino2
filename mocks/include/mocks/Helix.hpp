#pragma once

#include "providers/twitch/api/Helix.hpp"
#include "util/CancellationToken.hpp"

#include <gmock/gmock.h>
#include <QString>
#include <QStringList>

#include <functional>
#include <vector>

namespace chatterino::mock {

class Helix : public IHelix
{
public:
    virtual ~Helix() = default;

    MOCK_METHOD(void, fetchUsers,
                (QStringList userIds, QStringList userLogins,
                 ResultCallback<std::vector<HelixUser>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getUserByName,
                (QString userName, ResultCallback<HelixUser> successCallback,
                 HelixFailureCallback failureCallback),
                (override));
    MOCK_METHOD(void, getUserById,
                (QString userId, ResultCallback<HelixUser> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(
        void, getChannelFollowers,
        (QString broadcasterID,
         ResultCallback<HelixGetChannelFollowersResponse> successCallback,
         std::function<void(QString)> failureCallback),
        (override));

    MOCK_METHOD(void, fetchStreams,
                (QStringList userIds, QStringList userLogins,
                 ResultCallback<std::vector<HelixStream>> successCallback,
                 HelixFailureCallback failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, getStreamById,
                (QString userId,
                 (ResultCallback<bool, HelixStream> successCallback),
                 HelixFailureCallback failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, getStreamByName,
                (QString userName,
                 (ResultCallback<bool, HelixStream> successCallback),
                 HelixFailureCallback failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, fetchGames,
                (QStringList gameIds, QStringList gameNames,
                 (ResultCallback<std::vector<HelixGame>> successCallback),
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, searchGames,
                (QString gameName,
                 ResultCallback<std::vector<HelixGame>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getGameById,
                (QString gameId, ResultCallback<HelixGame> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, createClip,
                (QString channelId, ResultCallback<HelixClip> successCallback,
                 std::function<void(HelixClipError, QString)> failureCallback,
                 std::function<void()> finallyCallback),
                (override));

    MOCK_METHOD(void, fetchChannels,
                (QStringList userIDs,
                 ResultCallback<std::vector<HelixChannel>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getChannel,
                (QString broadcasterId,
                 ResultCallback<HelixChannel> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, createStreamMarker,
                (QString broadcasterId, QString description,
                 ResultCallback<HelixStreamMarker> successCallback,
                 std::function<void(HelixStreamMarkerError)> failureCallback),
                (override));

    MOCK_METHOD(void, loadBlocks,
                (QString userId,
                 ResultCallback<std::vector<HelixBlock>> successCallback,
                 FailureCallback<QString> failureCallback,
                 CancellationToken &&token),
                (override));

    MOCK_METHOD(void, blockUser,
                (QString targetUserId, const QObject *caller,
                 std::function<void()> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, unblockUser,
                (QString targetUserId, const QObject *caller,
                 std::function<void()> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(
        void, updateChannel,
        (QString broadcasterId, QString gameId, QString language, QString title,
         std::function<void(NetworkResult)> successCallback,
         (FailureCallback<HelixUpdateChannelError, QString> failureCallback)),
        (override));

    MOCK_METHOD(void, manageAutoModMessages,
                (QString userID, QString msgID, QString action,
                 std::function<void()> successCallback,
                 std::function<void(HelixAutoModMessageError)> failureCallback),
                (override));

    MOCK_METHOD(void, getCheermotes,
                (QString broadcasterId,
                 ResultCallback<std::vector<HelixCheermoteSet>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getEmoteSetData,
                (QString emoteSetId,
                 ResultCallback<HelixEmoteSetData> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    MOCK_METHOD(void, getChannelEmotes,
                (QString broadcasterId,
                 ResultCallback<std::vector<HelixChannelEmote>> successCallback,
                 HelixFailureCallback failureCallback),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, getGlobalBadges,
        (ResultCallback<HelixGlobalBadges> successCallback,
         (FailureCallback<HelixGetGlobalBadgesError, QString> failureCallback)),
        (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, getChannelBadges,
                (QString broadcasterID,
                 ResultCallback<HelixChannelBadges> successCallback,
                 (FailureCallback<HelixGetChannelBadgesError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateUserChatColor,
                (QString userID, QString color,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixUpdateUserChatColorError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, deleteChatMessages,
                (QString broadcasterID, QString moderatorID, QString messageID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixDeleteChatMessagesError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, addChannelModerator,
                (QString broadcasterID, QString userID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixAddChannelModeratorError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, removeChannelModerator,
                (QString broadcasterID, QString userID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixRemoveChannelModeratorError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, sendChatAnnouncement,
                (QString broadcasterID, QString moderatorID, QString message,
                 HelixAnnouncementColor color, ResultCallback<> successCallback,
                 (FailureCallback<HelixSendChatAnnouncementError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, addChannelVIP,
        (QString broadcasterID, QString userID,
         ResultCallback<> successCallback,
         (FailureCallback<HelixAddChannelVIPError, QString> failureCallback)),
        (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, removeChannelVIP,
                (QString broadcasterID, QString userID,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixRemoveChannelVIPError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, unbanUser,
        (QString broadcasterID, QString moderatorID, QString userID,
         ResultCallback<> successCallback,
         (FailureCallback<HelixUnbanUserError, QString> failureCallback)),
        (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(  // /raid
        void, startRaid,
        (QString fromBroadcasterID, QString toBroadcasterId,
         ResultCallback<> successCallback,
         (FailureCallback<HelixStartRaidError, QString> failureCallback)),
        (override));  // /raid

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(  // /unraid
        void, cancelRaid,
        (QString broadcasterID, ResultCallback<> successCallback,
         (FailureCallback<HelixCancelRaidError, QString> failureCallback)),
        (override));  // /unraid

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateEmoteMode,
                (QString broadcasterID, QString moderatorID, bool emoteMode,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateFollowerMode,
                (QString broadcasterID, QString moderatorID,
                 std::optional<int> followerModeDuration,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateNonModeratorChatDelay,
                (QString broadcasterID, QString moderatorID,
                 std::optional<int> nonModeratorChatDelayDuration,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateSlowMode,
                (QString broadcasterID, QString moderatorID,
                 std::optional<int> slowModeWaitTime,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateSubscriberMode,
                (QString broadcasterID, QString moderatorID,
                 bool subscriberMode,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateUniqueChatMode,
                (QString broadcasterID, QString moderatorID,
                 bool uniqueChatMode,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));
    // update chat settings

    // /timeout, /ban
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, banUser,
                (QString broadcasterID, QString moderatorID, QString userID,
                 std::optional<int> duration, QString reason,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixBanUserError, QString> failureCallback)),
                (override));  // /timeout, /ban

    // /warn
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, warnUser,
        (QString broadcasterID, QString moderatorID, QString userID,
         QString reason, ResultCallback<> successCallback,
         (FailureCallback<HelixWarnUserError, QString> failureCallback)),
        (override));  // /warn

    // /w
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, sendWhisper,
                (QString fromUserID, QString toUserID, QString message,
                 ResultCallback<> successCallback,
                 (FailureCallback<HelixWhisperError, QString> failureCallback)),
                (override));  // /w

    // getChatters
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, getChatters,
        (QString broadcasterID, QString moderatorID, size_t maxChattersToFetch,
         ResultCallback<HelixChatters> successCallback,
         (FailureCallback<HelixGetChattersError, QString> failureCallback)),
        (override));  // getChatters

    // /vips
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, getChannelVIPs,
        (QString broadcasterID,
         ResultCallback<std::vector<HelixVip>> successCallback,
         (FailureCallback<HelixListVIPsError, QString> failureCallback)),
        (override));  // /vips

    // /commercial
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, startCommercial,
        (QString broadcasterID, int length,
         ResultCallback<HelixStartCommercialResponse> successCallback,
         (FailureCallback<HelixStartCommercialError, QString> failureCallback)),
        (override));  // /commercial

    // /mods
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(
        void, getModerators,
        (QString broadcasterID, int maxModeratorsToFetch,
         ResultCallback<std::vector<HelixModerator>> successCallback,
         (FailureCallback<HelixGetModeratorsError, QString> failureCallback)),
        (override));  // /mods

    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateShieldMode,
                (QString broadcasterID, QString moderatorID, bool isActive,
                 ResultCallback<HelixShieldModeStatus> successCallback,
                 (FailureCallback<HelixUpdateShieldModeError, QString>
                      failureCallback)),
                (override));

    // /shoutout
    MOCK_METHOD(
        void, sendShoutout,
        (QString fromBroadcasterID, QString toBroadcasterID,
         QString moderatorID, ResultCallback<> successCallback,
         (FailureCallback<HelixSendShoutoutError, QString> failureCallback)),
        (override));

    // send message
    MOCK_METHOD(
        void, sendChatMessage,
        (HelixSendMessageArgs args,
         ResultCallback<HelixSentMessage> successCallback,
         (FailureCallback<HelixSendMessageError, QString> failureCallback)),
        (override));

    // get user emotes
    MOCK_METHOD(
        void, getUserEmotes,
        (QString userID, QString broadcasterID,
         (ResultCallback<std::vector<HelixChannelEmote>, HelixPaginationState>
              successCallback),
         FailureCallback<QString> failureCallback, CancellationToken &&token),
        (override));

    // get followed channel
    MOCK_METHOD(
        void, getFollowedChannel,
        (QString userID, QString broadcasterID, const QObject *caller,
         ResultCallback<std::optional<HelixFollowedChannel>> successCallback,
         FailureCallback<QString> failureCallback),
        (override));

    MOCK_METHOD(void, createEventSubSubscription,
                (const eventsub::SubscriptionRequest &request,
                 const QString &sessionID,
                 ResultCallback<HelixCreateEventSubSubscriptionResponse>
                     successCallback,
                 (FailureCallback<HelixCreateEventSubSubscriptionError, QString>
                      failureCallback)),
                (override));

    MOCK_METHOD(void, deleteEventSubSubscription,
                (const QString &request, ResultCallback<> successCallback,
                 (FailureCallback<QString> failureCallback)),
                (override));

    MOCK_METHOD(void, update, (QString clientId, QString oauthToken),
                (override));

protected:
    // The extra parenthesis around the failure callback is because its type
    // contains a comma
    MOCK_METHOD(void, updateChatSettings,
                (QString broadcasterID, QString moderatorID, QJsonObject json,
                 ResultCallback<HelixChatSettings> successCallback,
                 (FailureCallback<HelixUpdateChatSettingsError, QString>
                      failureCallback)),
                (override));
};

}  // namespace chatterino::mock
