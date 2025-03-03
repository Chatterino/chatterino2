#pragma once

#include "common/Aliases.hpp"
#include "common/network/NetworkRequest.hpp"
#include "providers/twitch/eventsub/SubscriptionRequest.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/Helpers.hpp"
#include "util/QStringHash.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QTimeZone>
#include <QUrl>
#include <QUrlQuery>

#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>

namespace chatterino {

using HelixFailureCallback = std::function<void()>;
template <typename... T>
using ResultCallback = std::function<void(T...)>;

class CancellationToken;

struct HelixUser {
    QString id;
    QString login;
    QString displayName;
    QString createdAt;
    QString description;
    QString profileImageUrl;

    explicit HelixUser(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , login(jsonObject.value("login").toString())
        , displayName(jsonObject.value("display_name").toString())
        , createdAt(jsonObject.value("created_at").toString())
        , description(jsonObject.value("description").toString())
        , profileImageUrl(jsonObject.value("profile_image_url").toString())
    {
    }
};

struct HelixGetChannelFollowersResponse {
    int total;

    explicit HelixGetChannelFollowersResponse(const QJsonObject &jsonObject)
        : total(jsonObject.value("total").toInt())
    {
    }
};

struct HelixStream {
    QString id;  // stream id
    QString userId;
    QString userLogin;
    QString userName;
    QString gameId;
    QString gameName;
    QString type;
    QString title;
    int viewerCount;
    QString startedAt;
    QString language;
    QString thumbnailUrl;

    // This is the names, the IDs are now always empty
    std::vector<QString> tags;

    HelixStream()
        : id("")
        , userId("")
        , userLogin("")
        , userName("")
        , gameId("")
        , gameName("")
        , type("")
        , title("")
        , viewerCount()
        , startedAt("")
        , language("")
        , thumbnailUrl("")
    {
    }

    explicit HelixStream(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , userId(jsonObject.value("user_id").toString())
        , userLogin(jsonObject.value("user_login").toString())
        , userName(jsonObject.value("user_name").toString())
        , gameId(jsonObject.value("game_id").toString())
        , gameName(jsonObject.value("game_name").toString())
        , type(jsonObject.value("type").toString())
        , title(jsonObject.value("title").toString())
        , viewerCount(jsonObject.value("viewer_count").toInt())
        , startedAt(jsonObject.value("started_at").toString())
        , language(jsonObject.value("language").toString())
        , thumbnailUrl(jsonObject.value("thumbnail_url").toString())
    {
        const auto jsonTags = jsonObject.value("tags").toArray();
        for (const auto &tag : jsonTags)
        {
            this->tags.push_back(tag.toString());
        }
    }
};

struct HelixGame {
    QString id;  // stream id
    QString name;
    QString boxArtUrl;

    explicit HelixGame(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , name(jsonObject.value("name").toString())
        , boxArtUrl(jsonObject.value("box_art_url").toString())
    {
    }
};

struct HelixClip {
    QString id;  // clip slug
    QString editUrl;

    explicit HelixClip(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , editUrl(jsonObject.value("edit_url").toString())
    {
    }
};

struct HelixChannel {
    QString userId;
    QString name;
    QString language;
    QString gameId;
    QString gameName;
    QString title;

    explicit HelixChannel(QJsonObject jsonObject)
        : userId(jsonObject.value("broadcaster_id").toString())
        , name(jsonObject.value("broadcaster_name").toString())
        , language(jsonObject.value("broadcaster_language").toString())
        , gameId(jsonObject.value("game_id").toString())
        , gameName(jsonObject.value("game_name").toString())
        , title(jsonObject.value("title").toString())
    {
    }
};

struct HelixStreamMarker {
    QString createdAt;
    QString description;
    QString id;
    int positionSeconds;

    explicit HelixStreamMarker(QJsonObject jsonObject)
        : createdAt(jsonObject.value("created_at").toString())
        , description(jsonObject.value("description").toString())
        , id(jsonObject.value("id").toString())
        , positionSeconds(jsonObject.value("position_seconds").toInt())
    {
    }
};

struct HelixBlock {
    QString userId;
    QString userName;
    QString displayName;

    explicit HelixBlock(QJsonObject jsonObject)
        : userId(jsonObject.value("user_id").toString())
        , userName(jsonObject.value("user_login").toString())
        , displayName(jsonObject.value("display_name").toString())
    {
    }
};

struct HelixCheermoteImage {
    Url imageURL1x;
    Url imageURL2x;
    Url imageURL4x;

    explicit HelixCheermoteImage(QJsonObject jsonObject)
        : imageURL1x(Url{jsonObject.value("1").toString()})
        , imageURL2x(Url{jsonObject.value("2").toString()})
        , imageURL4x(Url{jsonObject.value("4").toString()})
    {
    }
};

struct HelixCheermoteTier {
    QString id;
    QString color;
    int minBits;
    HelixCheermoteImage darkAnimated;
    HelixCheermoteImage darkStatic;
    HelixCheermoteImage lightAnimated;
    HelixCheermoteImage lightStatic;

    explicit HelixCheermoteTier(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , color(jsonObject.value("color").toString())
        , minBits(jsonObject.value("min_bits").toInt())
        , darkAnimated(jsonObject.value("images")
                           .toObject()
                           .value("dark")
                           .toObject()
                           .value("animated")
                           .toObject())
        , darkStatic(jsonObject.value("images")
                         .toObject()
                         .value("dark")
                         .toObject()
                         .value("static")
                         .toObject())
        , lightAnimated(jsonObject.value("images")
                            .toObject()
                            .value("light")
                            .toObject()
                            .value("animated")
                            .toObject())
        , lightStatic(jsonObject.value("images")
                          .toObject()
                          .value("light")
                          .toObject()
                          .value("static")
                          .toObject())
    {
    }
};

struct HelixCheermoteSet {
    QString prefix;
    QString type;
    std::vector<HelixCheermoteTier> tiers;

    explicit HelixCheermoteSet(QJsonObject jsonObject)
        : prefix(jsonObject.value("prefix").toString())
        , type(jsonObject.value("type").toString())
    {
        for (const auto &tier : jsonObject.value("tiers").toArray())
        {
            this->tiers.emplace_back(tier.toObject());
        }
    }
};

struct HelixEmoteSetData {
    QString setId;
    QString ownerId;
    QString emoteType;

    explicit HelixEmoteSetData(QJsonObject jsonObject)
        : setId(jsonObject.value("emote_set_id").toString())
        , ownerId(jsonObject.value("owner_id").toString())
        , emoteType(jsonObject.value("emote_type").toString())
    {
    }
};

struct HelixChannelEmote {
    const QString id;
    const QString name;
    const QString type;
    const QString setID;
    const QString ownerID;

    explicit HelixChannelEmote(const QJsonObject &jsonObject)
        : id(jsonObject["id"].toString())
        , name(jsonObject["name"].toString())
        , type(jsonObject["emote_type"].toString())
        , setID(jsonObject["emote_set_id"].toString())
        , ownerID(jsonObject["owner_id"].toString())
    {
    }
};

struct HelixChatSettings {
    const QString broadcasterId;
    const bool emoteMode;
    // std::nullopt if disabled
    const std::optional<int> followerModeDuration;           // time in minutes
    const std::optional<int> nonModeratorChatDelayDuration;  // time in seconds
    const std::optional<int> slowModeWaitTime;               // time in seconds
    const bool subscriberMode;
    const bool uniqueChatMode;

    explicit HelixChatSettings(QJsonObject jsonObject)
        : broadcasterId(jsonObject.value("broadcaster_id").toString())
        , emoteMode(jsonObject.value("emote_mode").toBool())
        , followerModeDuration(makeConditionedOptional(
              jsonObject.value("follower_mode").toBool(),
              jsonObject.value("follower_mode_duration").toInt()))
        , nonModeratorChatDelayDuration(makeConditionedOptional(
              jsonObject.value("non_moderator_chat_delay").toBool(),
              jsonObject.value("non_moderator_chat_delay_duration").toInt()))
        , slowModeWaitTime(makeConditionedOptional(
              jsonObject.value("slow_mode").toBool(),
              jsonObject.value("slow_mode_wait_time").toInt()))
        , subscriberMode(jsonObject.value("subscriber_mode").toBool())
        , uniqueChatMode(jsonObject.value("unique_chat_mode").toBool())
    {
    }
};

struct HelixVip {
    // Twitch ID of the user
    QString userId;

    // Display name of the user
    QString userName;

    // Login name of the user
    QString userLogin;

    explicit HelixVip(const QJsonObject &jsonObject)
        : userId(jsonObject.value("user_id").toString())
        , userName(jsonObject.value("user_name").toString())
        , userLogin(jsonObject.value("user_login").toString())
    {
    }
};

struct HelixChatters {
    std::unordered_set<QString> chatters;
    int total{};
    QString cursor;

    HelixChatters() = default;

    explicit HelixChatters(const QJsonObject &jsonObject);
};

using HelixModerator = HelixVip;

struct HelixModerators {
    std::vector<HelixModerator> moderators;
    QString cursor;

    HelixModerators() = default;

    explicit HelixModerators(const QJsonObject &jsonObject)
        : cursor(jsonObject.value("pagination")
                     .toObject()
                     .value("cursor")
                     .toString())
    {
        const auto &data = jsonObject.value("data").toArray();
        for (const auto &mod : data)
        {
            HelixModerator moderator(mod.toObject());

            this->moderators.push_back(moderator);
        }
    }
};

struct HelixBadgeVersion {
    QString id;
    Url imageURL1x;
    Url imageURL2x;
    Url imageURL4x;
    QString title;
    Url clickURL;

    explicit HelixBadgeVersion(const QJsonObject &jsonObject)
        : id(jsonObject.value("id").toString())
        , imageURL1x(Url{jsonObject.value("image_url_1x").toString()})
        , imageURL2x(Url{jsonObject.value("image_url_2x").toString()})
        , imageURL4x(Url{jsonObject.value("image_url_4x").toString()})
        , title(jsonObject.value("title").toString())
        , clickURL(Url{jsonObject.value("click_url").toString()})
    {
    }
};

struct HelixBadgeSet {
    QString setID;
    std::vector<HelixBadgeVersion> versions;

    explicit HelixBadgeSet(const QJsonObject &json)
        : setID(json.value("set_id").toString())
    {
        const auto jsonVersions = json.value("versions").toArray();
        for (const auto &version : jsonVersions)
        {
            versions.emplace_back(version.toObject());
        }
    }
};

struct HelixGlobalBadges {
    std::vector<HelixBadgeSet> badgeSets;

    explicit HelixGlobalBadges(const QJsonObject &jsonObject)
    {
        const auto &data = jsonObject.value("data").toArray();
        for (const auto &set : data)
        {
            this->badgeSets.emplace_back(set.toObject());
        }
    }
};

using HelixChannelBadges = HelixGlobalBadges;

struct HelixDropReason {
    QString code;
    QString message;

    explicit HelixDropReason(const QJsonObject &jsonObject)
        : code(jsonObject["code"].toString())
        , message(jsonObject["message"].toString())
    {
    }
};

struct HelixSentMessage {
    QString id;
    bool isSent;
    std::optional<HelixDropReason> dropReason;

    explicit HelixSentMessage(const QJsonObject &jsonObject)
        : id(jsonObject["message_id"].toString())
        , isSent(jsonObject["is_sent"].toBool())
        , dropReason(jsonObject.contains("drop_reason")
                         ? std::optional(HelixDropReason(
                               jsonObject["drop_reason"].toObject()))
                         : std::nullopt)
    {
    }
};

struct HelixFollowedChannel {
    QString broadcasterID;
    QString broadcasterLogin;
    QString broadcasterName;
    QDateTime followedAt;

    explicit HelixFollowedChannel(const QJsonObject &jsonObject)
        : broadcasterID(jsonObject["broadcaster_id"].toString())
        , broadcasterLogin(jsonObject["broadcaster_login"].toString())
        , broadcasterName(jsonObject["broadcaster_name"].toString())
        , followedAt(QDateTime::fromString(jsonObject["followed_at"].toString(),
                                           Qt::ISODate))
    {
    }
};

struct HelixSendMessageArgs {
    QString broadcasterID;
    QString senderID;
    QString message;
    /// Optional
    QString replyParentMessageID;
};

enum class HelixAnnouncementColor {
    Blue,
    Green,
    Orange,
    Purple,

    // this is the executor's chat color
    Primary,
};

enum class HelixClipError {
    Unknown,
    ClipsUnavailable,
    ClipsDisabled,
    ClipsRestricted,
    ClipsRestrictedCategory,
    UserNotAuthenticated,
};

enum class HelixStreamMarkerError {
    Unknown,
    UserNotAuthorized,
    UserNotAuthenticated,
};

enum class HelixAutoModMessageError {
    Unknown,
    MessageAlreadyProcessed,
    UserNotAuthenticated,
    UserNotAuthorized,
    MessageNotFound,
};

enum class HelixUpdateUserChatColorError {
    Unknown,
    UserMissingScope,
    InvalidColor,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixDeleteChatMessagesError {
    Unknown,
    UserMissingScope,
    UserNotAuthenticated,
    UserNotAuthorized,
    MessageUnavailable,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixSendChatAnnouncementError {
    Unknown,
    UserMissingScope,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixAddChannelModeratorError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    TargetAlreadyModded,
    TargetIsVIP,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixRemoveChannelModeratorError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    TargetNotModded,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixAddChannelVIPError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixRemoveChannelVIPError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

// These changes are from the helix-command-migration/unban-untimeout branch
enum class HelixUnbanUserError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    ConflictingOperation,
    TargetNotBanned,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // These changes are from the helix-command-migration/unban-untimeout branch

enum class HelixStartRaidError {  // /raid
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    CantRaidYourself,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // /raid

enum class HelixCancelRaidError {  // /unraid
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    NoRaidPending,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // /unraid

enum class HelixUpdateChatSettingsError {  // update chat settings
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    Forbidden,
    OutOfRange,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // update chat settings

/// Error type for Helix::updateChannel
///
/// Used in the /settitle and /setgame commands
enum class HelixUpdateChannelError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixBanUserError {  // /timeout, /ban
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    ConflictingOperation,
    TargetBanned,
    CannotBanUser,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // /timeout, /ban

enum class HelixWarnUserError {  // /warn
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    ConflictingOperation,
    CannotWarnUser,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // /warn

enum class HelixWhisperError {  // /w
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    Ratelimited,
    NoVerifiedPhone,
    RecipientBlockedUser,
    WhisperSelf,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // /w

enum class HelixGetChattersError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixGetModeratorsError {
    Unknown,
    UserMissingScope,
    UserNotAuthorized,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixListVIPsError {  // /vips
    Unknown,
    UserMissingScope,
    UserNotAuthorized,
    UserNotBroadcaster,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};  // /vips

enum class HelixSendShoutoutError {
    Unknown,
    // 400
    UserIsBroadcaster,
    BroadcasterNotLive,
    // 401
    UserNotAuthorized,
    UserMissingScope,

    Ratelimited,
};

struct HelixStartCommercialResponse {
    // Length of the triggered commercial
    int length;
    // Provides contextual information on why the request failed
    QString message;
    // Seconds until the next commercial can be served on this channel
    int retryAfter;

    explicit HelixStartCommercialResponse(const QJsonObject &jsonObject)
    {
        auto jsonData = jsonObject.value("data").toArray().at(0).toObject();
        this->length = jsonData.value("length").toInt();
        this->message = jsonData.value("message").toString();
        this->retryAfter = jsonData.value("retry_after").toInt();
    }
};

struct HelixShieldModeStatus {
    /// A Boolean value that determines whether Shield Mode is active. Is `true` if Shield Mode is active; otherwise, `false`.
    bool isActive;
    /// An ID that identifies the moderator that last activated Shield Mode.
    QString moderatorID;
    /// The moderator's login name.
    QString moderatorLogin;
    /// The moderator's display name.
    QString moderatorName;
    /// The UTC timestamp of when Shield Mode was last activated.
    QDateTime lastActivatedAt;

    explicit HelixShieldModeStatus(const QJsonObject &json)
        : isActive(json["is_active"].toBool())
        , moderatorID(json["moderator_id"].toString())
        , moderatorLogin(json["moderator_login"].toString())
        , moderatorName(json["moderator_name"].toString())
        , lastActivatedAt(QDateTime::fromString(
              json["last_activated_at"].toString(), Qt::ISODate))
    {
        this->lastActivatedAt.setTimeZone(QTimeZone::utc());
    }
};

enum class HelixUpdateShieldModeError {
    Unknown,
    UserMissingScope,
    MissingPermission,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixStartCommercialError {
    Unknown,
    TokenMustMatchBroadcaster,
    UserMissingScope,
    BroadcasterNotStreaming,
    MissingLengthParameter,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixGetGlobalBadgesError {
    Unknown,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

enum class HelixSendMessageError {
    Unknown,

    MissingText,
    BadRequest,
    Forbidden,
    MessageTooLarge,
    UserMissingScope,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

struct HelixError {
    /// Text version of the HTTP error that happened (e.g. Bad Request)
    QString error;
    /// Number version of the HTTP error that happened (e.g. 400)
    int status;
    /// The error message string
    QString message;

    explicit HelixError(const QJsonObject &json)
        : error(json["error"].toString())
        , status(json["status"].toInt())
        , message(json["message"].toString())
    {
    }
};

using HelixGetChannelBadgesError = HelixGetGlobalBadgesError;

struct HelixPaginationState {
    bool done;
};

struct HelixCreateEventSubSubscriptionResponse {
    QString subscriptionID;
    QString subscriptionStatus;
    QString subscriptionType;
    QString subscriptionVersion;
    QJsonObject subscriptionCondition;
    QString subscriptionCreatedAt;
    QString subscriptionSessionID;
    QString subscriptionConnectedAt;
    int subscriptionCost;

    int total;
    int totalCost;
    int maxTotalCost;

    explicit HelixCreateEventSubSubscriptionResponse(
        const QJsonObject &jsonObject)
    {
        {
            auto jsonData = jsonObject.value("data").toArray().at(0).toObject();
            this->subscriptionID = jsonData.value("id").toString();
            this->subscriptionStatus = jsonData.value("status").toString();
            this->subscriptionType = jsonData.value("type").toString();
            this->subscriptionVersion = jsonData.value("version").toString();
            this->subscriptionCondition =
                jsonData.value("condition").toObject();
            this->subscriptionCreatedAt =
                jsonData.value("created_at").toString();
            this->subscriptionSessionID = jsonData.value("transport")
                                              .toObject()
                                              .value("session_id")
                                              .toString();
            this->subscriptionConnectedAt = jsonData.value("transport")
                                                .toObject()
                                                .value("connected_at")
                                                .toString();
            this->subscriptionCost = jsonData.value("cost").toInt();
        }

        this->total = jsonObject.value("total").toInt();
        this->totalCost = jsonObject.value("total_cost").toInt();
        this->maxTotalCost = jsonObject.value("max_total_cost").toInt();
    }

    friend QDebug &operator<<(
        QDebug &dbg, const HelixCreateEventSubSubscriptionResponse &data);
};

enum class HelixCreateEventSubSubscriptionError : std::uint8_t {
    BadRequest,
    Unauthorized,
    Forbidden,
    Conflict,
    Ratelimited,

    // The error message is forwarded directly from the Twitch API
    Forwarded,
};

class IHelix
{
public:
    template <typename... T>
    using FailureCallback = std::function<void(T...)>;

    // https://dev.twitch.tv/docs/api/reference#get-users
    virtual void fetchUsers(
        QStringList userIds, QStringList userLogins,
        ResultCallback<std::vector<HelixUser>> successCallback,
        HelixFailureCallback failureCallback) = 0;
    virtual void getUserByName(QString userName,
                               ResultCallback<HelixUser> successCallback,
                               HelixFailureCallback failureCallback) = 0;
    virtual void getUserById(QString userId,
                             ResultCallback<HelixUser> successCallback,
                             HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#get-channel-followers
    virtual void getChannelFollowers(
        QString broadcasterID,
        ResultCallback<HelixGetChannelFollowersResponse> successCallback,
        std::function<void(QString)> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-streams
    virtual void fetchStreams(
        QStringList userIds, QStringList userLogins,
        ResultCallback<std::vector<HelixStream>> successCallback,
        HelixFailureCallback failureCallback,
        std::function<void()> finallyCallback) = 0;

    virtual void getStreamById(
        QString userId, ResultCallback<bool, HelixStream> successCallback,
        HelixFailureCallback failureCallback,
        std::function<void()> finallyCallback) = 0;

    virtual void getStreamByName(
        QString userName, ResultCallback<bool, HelixStream> successCallback,
        HelixFailureCallback failureCallback,
        std::function<void()> finallyCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-games
    virtual void fetchGames(
        QStringList gameIds, QStringList gameNames,
        ResultCallback<std::vector<HelixGame>> successCallback,
        HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#search-categories
    virtual void searchGames(
        QString gameName,
        ResultCallback<std::vector<HelixGame>> successCallback,
        HelixFailureCallback failureCallback) = 0;

    virtual void getGameById(QString gameId,
                             ResultCallback<HelixGame> successCallback,
                             HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#create-clip
    virtual void createClip(
        QString channelId, ResultCallback<HelixClip> successCallback,
        std::function<void(HelixClipError, QString)> failureCallback,
        std::function<void()> finallyCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-channel-information
    virtual void fetchChannels(
        QStringList userIDs,
        ResultCallback<std::vector<HelixChannel>> successCallback,
        HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-channel-information
    virtual void getChannel(QString broadcasterId,
                            ResultCallback<HelixChannel> successCallback,
                            HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#create-stream-marker
    virtual void createStreamMarker(
        QString broadcasterId, QString description,
        ResultCallback<HelixStreamMarker> successCallback,
        std::function<void(HelixStreamMarkerError)> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-user-block-list
    virtual void loadBlocks(
        QString userId, ResultCallback<std::vector<HelixBlock>> pageCallback,
        FailureCallback<QString> failureCallback,
        CancellationToken &&token) = 0;

    // https://dev.twitch.tv/docs/api/reference#block-user
    virtual void blockUser(QString targetUserId, const QObject *caller,
                           std::function<void()> successCallback,
                           HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#unblock-user
    virtual void unblockUser(QString targetUserId, const QObject *caller,
                             std::function<void()> successCallback,
                             HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#modify-channel-information
    virtual void updateChannel(
        QString broadcasterId, QString gameId, QString language, QString title,
        std::function<void(NetworkResult)> successCallback,
        FailureCallback<HelixUpdateChannelError, QString> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages
    virtual void manageAutoModMessages(
        QString userID, QString msgID, QString action,
        std::function<void()> successCallback,
        std::function<void(HelixAutoModMessageError)> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#get-cheermotes
    virtual void getCheermotes(
        QString broadcasterId,
        ResultCallback<std::vector<HelixCheermoteSet>> successCallback,
        HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-emote-sets
    virtual void getEmoteSetData(
        QString emoteSetId, ResultCallback<HelixEmoteSetData> successCallback,
        HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-channel-emotes
    virtual void getChannelEmotes(
        QString broadcasterId,
        ResultCallback<std::vector<HelixChannelEmote>> successCallback,
        HelixFailureCallback failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#update-user-chat-color
    virtual void updateUserChatColor(
        QString userID, QString color, ResultCallback<> successCallback,
        FailureCallback<HelixUpdateUserChatColorError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#delete-chat-messages
    virtual void deleteChatMessages(
        QString broadcasterID, QString moderatorID, QString messageID,
        ResultCallback<> successCallback,
        FailureCallback<HelixDeleteChatMessagesError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#add-channel-moderator
    virtual void addChannelModerator(
        QString broadcasterID, QString userID, ResultCallback<> successCallback,
        FailureCallback<HelixAddChannelModeratorError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#remove-channel-moderator
    virtual void removeChannelModerator(
        QString broadcasterID, QString userID, ResultCallback<> successCallback,
        FailureCallback<HelixRemoveChannelModeratorError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#send-chat-announcement
    virtual void sendChatAnnouncement(
        QString broadcasterID, QString moderatorID, QString message,
        HelixAnnouncementColor color, ResultCallback<> successCallback,
        FailureCallback<HelixSendChatAnnouncementError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#add-channel-vip
    virtual void addChannelVIP(
        QString broadcasterID, QString userID, ResultCallback<> successCallback,
        FailureCallback<HelixAddChannelVIPError, QString> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#remove-channel-vip
    virtual void removeChannelVIP(
        QString broadcasterID, QString userID, ResultCallback<> successCallback,
        FailureCallback<HelixRemoveChannelVIPError, QString>
            failureCallback) = 0;

    // These changes are from the helix-command-migration/unban-untimeout branch
    // https://dev.twitch.tv/docs/api/reference#unban-user
    // These changes are from the helix-command-migration/unban-untimeout branch
    virtual void unbanUser(
        QString broadcasterID, QString moderatorID, QString userID,
        ResultCallback<> successCallback,
        FailureCallback<HelixUnbanUserError, QString> failureCallback) = 0;
    // These changes are from the helix-command-migration/unban-untimeout branch

    // https://dev.twitch.tv/docs/api/reference#start-a-raid
    virtual void startRaid(
        QString fromBroadcasterID, QString toBroadcasterID,
        ResultCallback<> successCallback,
        FailureCallback<HelixStartRaidError, QString> failureCallback) = 0;
    // https://dev.twitch.tv/docs/api/reference#start-a-raid

    // https://dev.twitch.tv/docs/api/reference#cancel-a-raid
    virtual void cancelRaid(
        QString broadcasterID, ResultCallback<> successCallback,
        FailureCallback<HelixCancelRaidError, QString> failureCallback) = 0;
    // https://dev.twitch.tv/docs/api/reference#cancel-a-raid

    // Updates the emote mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateEmoteMode(
        QString broadcasterID, QString moderatorID, bool emoteMode,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;

    // Updates the follower mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateFollowerMode(
        QString broadcasterID, QString moderatorID,
        std::optional<int> followerModeDuration,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;

    // Updates the non-moderator chat delay using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateNonModeratorChatDelay(
        QString broadcasterID, QString moderatorID,
        std::optional<int> nonModeratorChatDelayDuration,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;

    // Updates the slow mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateSlowMode(
        QString broadcasterID, QString moderatorID,
        std::optional<int> slowModeWaitTime,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;

    // Updates the subscriber mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateSubscriberMode(
        QString broadcasterID, QString moderatorID, bool subscriberMode,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;

    // Updates the unique chat mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateUniqueChatMode(
        QString broadcasterID, QString moderatorID, bool uniqueChatMode,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;

    // Ban/timeout a user
    // https://dev.twitch.tv/docs/api/reference#ban-user
    virtual void banUser(
        QString broadcasterID, QString moderatorID, QString userID,
        std::optional<int> duration, QString reason,
        ResultCallback<> successCallback,
        FailureCallback<HelixBanUserError, QString> failureCallback) = 0;

    // Warn a user
    // https://dev.twitch.tv/docs/api/reference#warn-chat-user
    virtual void warnUser(
        QString broadcasterID, QString moderatorID, QString userID,
        QString reason, ResultCallback<> successCallback,
        FailureCallback<HelixWarnUserError, QString> failureCallback) = 0;

    // Send a whisper
    // https://dev.twitch.tv/docs/api/reference#send-whisper
    virtual void sendWhisper(
        QString fromUserID, QString toUserID, QString message,
        ResultCallback<> successCallback,
        FailureCallback<HelixWhisperError, QString> failureCallback) = 0;

    // Get Chatters from the `broadcasterID` channel
    // This will follow the returned cursor and return up to `maxChattersToFetch` chatters
    // https://dev.twitch.tv/docs/api/reference#get-chatters
    virtual void getChatters(
        QString broadcasterID, QString moderatorID, size_t maxChattersToFetch,
        ResultCallback<HelixChatters> successCallback,
        FailureCallback<HelixGetChattersError, QString> failureCallback) = 0;

    // Get moderators from the `broadcasterID` channel
    // This will follow the returned cursor
    // https://dev.twitch.tv/docs/api/reference#get-moderators
    virtual void getModerators(
        QString broadcasterID, int maxModeratorsToFetch,
        ResultCallback<std::vector<HelixModerator>> successCallback,
        FailureCallback<HelixGetModeratorsError, QString> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#get-vips
    virtual void getChannelVIPs(
        QString broadcasterID,
        ResultCallback<std::vector<HelixVip>> successCallback,
        FailureCallback<HelixListVIPsError, QString> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference#start-commercial
    virtual void startCommercial(
        QString broadcasterID, int length,
        ResultCallback<HelixStartCommercialResponse> successCallback,
        FailureCallback<HelixStartCommercialError, QString>
            failureCallback) = 0;

    // Get global Twitch badges
    // https://dev.twitch.tv/docs/api/reference/#get-global-chat-badges
    virtual void getGlobalBadges(
        ResultCallback<HelixGlobalBadges> successCallback,
        FailureCallback<HelixGetGlobalBadgesError, QString>
            failureCallback) = 0;

    // Get badges for the `broadcasterID` channel
    // https://dev.twitch.tv/docs/api/reference/#get-channel-chat-badges
    virtual void getChannelBadges(
        QString broadcasterID,
        ResultCallback<HelixChannelBadges> successCallback,
        FailureCallback<HelixGetChannelBadgesError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#update-shield-mode-status
    virtual void updateShieldMode(
        QString broadcasterID, QString moderatorID, bool isActive,
        ResultCallback<HelixShieldModeStatus> successCallback,
        FailureCallback<HelixUpdateShieldModeError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#send-a-shoutout
    virtual void sendShoutout(
        QString fromBroadcasterID, QString toBroadcasterID, QString moderatorID,
        ResultCallback<> successCallback,
        FailureCallback<HelixSendShoutoutError, QString> failureCallback) = 0;

    /// https://dev.twitch.tv/docs/api/reference/#send-chat-message
    virtual void sendChatMessage(
        HelixSendMessageArgs args,
        ResultCallback<HelixSentMessage> successCallback,
        FailureCallback<HelixSendMessageError, QString> failureCallback) = 0;

    /// https://dev.twitch.tv/docs/api/reference/#get-user-emotes
    virtual void getUserEmotes(
        QString userID, QString broadcasterID,
        ResultCallback<std::vector<HelixChannelEmote>, HelixPaginationState>
            pageCallback,
        FailureCallback<QString> failureCallback,
        CancellationToken &&token) = 0;

    /// https://dev.twitch.tv/docs/api/reference/#get-followed-channels
    /// (non paginated)
    virtual void getFollowedChannel(
        QString userID, QString broadcasterID, const QObject *caller,
        ResultCallback<std::optional<HelixFollowedChannel>> successCallback,
        FailureCallback<QString> failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#create-eventsub-subscription
    virtual void createEventSubSubscription(
        const eventsub::SubscriptionRequest &request, const QString &sessionID,
        ResultCallback<HelixCreateEventSubSubscriptionResponse> successCallback,
        FailureCallback<HelixCreateEventSubSubscriptionError, QString>
            failureCallback) = 0;

    // https://dev.twitch.tv/docs/api/reference/#delete-eventsub-subscription
    virtual void deleteEventSubSubscription(
        const QString &subscriptionID, ResultCallback<> successCallback,
        FailureCallback<QString> failureCallback) = 0;

    virtual void update(QString clientId, QString oauthToken) = 0;

protected:
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    virtual void updateChatSettings(
        QString broadcasterID, QString moderatorID, QJsonObject json,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString>
            failureCallback) = 0;
};

class Helix final : public IHelix
{
public:
    // https://dev.twitch.tv/docs/api/reference#get-users
    void fetchUsers(QStringList userIds, QStringList userLogins,
                    ResultCallback<std::vector<HelixUser>> successCallback,
                    HelixFailureCallback failureCallback) final;
    void getUserByName(QString userName,
                       ResultCallback<HelixUser> successCallback,
                       HelixFailureCallback failureCallback) final;
    void getUserById(QString userId, ResultCallback<HelixUser> successCallback,
                     HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#get-channel-followers
    void getChannelFollowers(
        QString broadcasterID,
        ResultCallback<HelixGetChannelFollowersResponse> successCallback,
        std::function<void(QString)> failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-streams
    void fetchStreams(QStringList userIds, QStringList userLogins,
                      ResultCallback<std::vector<HelixStream>> successCallback,
                      HelixFailureCallback failureCallback,
                      std::function<void()> finallyCallback) final;

    void getStreamById(QString userId,
                       ResultCallback<bool, HelixStream> successCallback,
                       HelixFailureCallback failureCallback,
                       std::function<void()> finallyCallback) final;

    void getStreamByName(QString userName,
                         ResultCallback<bool, HelixStream> successCallback,
                         HelixFailureCallback failureCallback,
                         std::function<void()> finallyCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-games
    void fetchGames(QStringList gameIds, QStringList gameNames,
                    ResultCallback<std::vector<HelixGame>> successCallback,
                    HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#search-categories
    void searchGames(QString gameName,
                     ResultCallback<std::vector<HelixGame>> successCallback,
                     HelixFailureCallback failureCallback) final;

    void getGameById(QString gameId, ResultCallback<HelixGame> successCallback,
                     HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#create-clip
    void createClip(
        QString channelId, ResultCallback<HelixClip> successCallback,
        std::function<void(HelixClipError, QString)> failureCallback,
        std::function<void()> finallyCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-channel-information
    void fetchChannels(
        QStringList userIDs,
        ResultCallback<std::vector<HelixChannel>> successCallback,
        HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-channel-information
    void getChannel(QString broadcasterId,
                    ResultCallback<HelixChannel> successCallback,
                    HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#create-stream-marker
    void createStreamMarker(
        QString broadcasterId, QString description,
        ResultCallback<HelixStreamMarker> successCallback,
        std::function<void(HelixStreamMarkerError)> failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-user-block-list
    void loadBlocks(QString userId,
                    ResultCallback<std::vector<HelixBlock>> pageCallback,
                    FailureCallback<QString> failureCallback,
                    CancellationToken &&token) final;

    // https://dev.twitch.tv/docs/api/reference#block-user
    void blockUser(QString targetUserId, const QObject *caller,
                   std::function<void()> successCallback,
                   HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#unblock-user
    void unblockUser(QString targetUserId, const QObject *caller,
                     std::function<void()> successCallback,
                     HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#modify-channel-information
    void updateChannel(QString broadcasterId, QString gameId, QString language,
                       QString title,
                       std::function<void(NetworkResult)> successCallback,
                       FailureCallback<HelixUpdateChannelError, QString>
                           failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages
    void manageAutoModMessages(
        QString userID, QString msgID, QString action,
        std::function<void()> successCallback,
        std::function<void(HelixAutoModMessageError)> failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#get-cheermotes
    void getCheermotes(
        QString broadcasterId,
        ResultCallback<std::vector<HelixCheermoteSet>> successCallback,
        HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-emote-sets
    void getEmoteSetData(QString emoteSetId,
                         ResultCallback<HelixEmoteSetData> successCallback,
                         HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#get-channel-emotes
    void getChannelEmotes(
        QString broadcasterId,
        ResultCallback<std::vector<HelixChannelEmote>> successCallback,
        HelixFailureCallback failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#update-user-chat-color
    void updateUserChatColor(
        QString userID, QString color, ResultCallback<> successCallback,
        FailureCallback<HelixUpdateUserChatColorError, QString> failureCallback)
        final;

    // https://dev.twitch.tv/docs/api/reference#delete-chat-messages
    void deleteChatMessages(
        QString broadcasterID, QString moderatorID, QString messageID,
        ResultCallback<> successCallback,
        FailureCallback<HelixDeleteChatMessagesError, QString> failureCallback)
        final;

    // https://dev.twitch.tv/docs/api/reference#add-channel-moderator
    void addChannelModerator(
        QString broadcasterID, QString userID, ResultCallback<> successCallback,
        FailureCallback<HelixAddChannelModeratorError, QString> failureCallback)
        final;

    // https://dev.twitch.tv/docs/api/reference#remove-channel-moderator
    void removeChannelModerator(
        QString broadcasterID, QString userID, ResultCallback<> successCallback,
        FailureCallback<HelixRemoveChannelModeratorError, QString>
            failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#send-chat-announcement
    void sendChatAnnouncement(
        QString broadcasterID, QString moderatorID, QString message,
        HelixAnnouncementColor color, ResultCallback<> successCallback,
        FailureCallback<HelixSendChatAnnouncementError, QString>
            failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#add-channel-vip
    void addChannelVIP(QString broadcasterID, QString userID,
                       ResultCallback<> successCallback,
                       FailureCallback<HelixAddChannelVIPError, QString>
                           failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#remove-channel-vip
    void removeChannelVIP(QString broadcasterID, QString userID,
                          ResultCallback<> successCallback,
                          FailureCallback<HelixRemoveChannelVIPError, QString>
                              failureCallback) final;

    // These changes are from the helix-command-migration/unban-untimeout branch
    // https://dev.twitch.tv/docs/api/reference#unban-user
    // These changes are from the helix-command-migration/unban-untimeout branch
    void unbanUser(
        QString broadcasterID, QString moderatorID, QString userID,
        ResultCallback<> successCallback,
        FailureCallback<HelixUnbanUserError, QString> failureCallback) final;
    // These changes are from the helix-command-migration/unban-untimeout branch

    // https://dev.twitch.tv/docs/api/reference#start-a-raid
    void startRaid(
        QString fromBroadcasterID, QString toBroadcasterID,
        ResultCallback<> successCallback,
        FailureCallback<HelixStartRaidError, QString> failureCallback) final;
    // https://dev.twitch.tv/docs/api/reference#start-a-raid

    // https://dev.twitch.tv/docs/api/reference#cancel-a-raid
    void cancelRaid(
        QString broadcasterID, ResultCallback<> successCallback,
        FailureCallback<HelixCancelRaidError, QString> failureCallback) final;
    // https://dev.twitch.tv/docs/api/reference#cancel-a-raid

    // Updates the emote mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateEmoteMode(QString broadcasterID, QString moderatorID,
                         bool emoteMode,
                         ResultCallback<HelixChatSettings> successCallback,
                         FailureCallback<HelixUpdateChatSettingsError, QString>
                             failureCallback) final;

    // Updates the follower mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateFollowerMode(
        QString broadcasterID, QString moderatorID,
        std::optional<int> followerModeDuration,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
        final;

    // Updates the non-moderator chat delay using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateNonModeratorChatDelay(
        QString broadcasterID, QString moderatorID,
        std::optional<int> nonModeratorChatDelayDuration,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
        final;

    // Updates the slow mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateSlowMode(QString broadcasterID, QString moderatorID,
                        std::optional<int> slowModeWaitTime,
                        ResultCallback<HelixChatSettings> successCallback,
                        FailureCallback<HelixUpdateChatSettingsError, QString>
                            failureCallback) final;

    // Updates the subscriber mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateSubscriberMode(
        QString broadcasterID, QString moderatorID, bool subscriberMode,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
        final;

    // Updates the unique chat mode using
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateUniqueChatMode(
        QString broadcasterID, QString moderatorID, bool uniqueChatMode,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
        final;

    // Ban/timeout a user
    // https://dev.twitch.tv/docs/api/reference#ban-user
    void banUser(
        QString broadcasterID, QString moderatorID, QString userID,
        std::optional<int> duration, QString reason,
        ResultCallback<> successCallback,
        FailureCallback<HelixBanUserError, QString> failureCallback) final;

    // Warn a user
    // https://dev.twitch.tv/docs/api/reference#warn-chat-user
    void warnUser(
        QString broadcasterID, QString moderatorID, QString userID,
        QString reason, ResultCallback<> successCallback,
        FailureCallback<HelixWarnUserError, QString> failureCallback) final;

    // Send a whisper
    // https://dev.twitch.tv/docs/api/reference#send-whisper
    void sendWhisper(
        QString fromUserID, QString toUserID, QString message,
        ResultCallback<> successCallback,
        FailureCallback<HelixWhisperError, QString> failureCallback) final;

    // Get Chatters from the `broadcasterID` channel
    // This will follow the returned cursor and return up to `maxChattersToFetch` chatters
    // https://dev.twitch.tv/docs/api/reference#get-chatters
    void getChatters(
        QString broadcasterID, QString moderatorID, size_t maxChattersToFetch,
        ResultCallback<HelixChatters> successCallback,
        FailureCallback<HelixGetChattersError, QString> failureCallback) final;

    // Get moderators from the `broadcasterID` channel
    // This will follow the returned cursor
    // https://dev.twitch.tv/docs/api/reference#get-moderators
    void getModerators(
        QString broadcasterID, int maxModeratorsToFetch,
        ResultCallback<std::vector<HelixModerator>> successCallback,
        FailureCallback<HelixGetModeratorsError, QString> failureCallback)
        final;

    // https://dev.twitch.tv/docs/api/reference#get-vips
    void getChannelVIPs(
        QString broadcasterID,
        ResultCallback<std::vector<HelixVip>> successCallback,
        FailureCallback<HelixListVIPsError, QString> failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference#start-commercial
    void startCommercial(
        QString broadcasterID, int length,
        ResultCallback<HelixStartCommercialResponse> successCallback,
        FailureCallback<HelixStartCommercialError, QString> failureCallback)
        final;

    // Get global Twitch badges
    // https://dev.twitch.tv/docs/api/reference/#get-global-chat-badges
    void getGlobalBadges(ResultCallback<HelixGlobalBadges> successCallback,
                         FailureCallback<HelixGetGlobalBadgesError, QString>
                             failureCallback) final;

    // Get badges for the `broadcasterID` channel
    // https://dev.twitch.tv/docs/api/reference/#get-channel-chat-badges
    void getChannelBadges(QString broadcasterID,
                          ResultCallback<HelixChannelBadges> successCallback,
                          FailureCallback<HelixGetChannelBadgesError, QString>
                              failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#update-shield-mode-status
    void updateShieldMode(QString broadcasterID, QString moderatorID,
                          bool isActive,
                          ResultCallback<HelixShieldModeStatus> successCallback,
                          FailureCallback<HelixUpdateShieldModeError, QString>
                              failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#send-a-shoutout
    void sendShoutout(
        QString fromBroadcasterID, QString toBroadcasterID, QString moderatorID,
        ResultCallback<> successCallback,
        FailureCallback<HelixSendShoutoutError, QString> failureCallback) final;

    /// https://dev.twitch.tv/docs/api/reference/#send-chat-message
    void sendChatMessage(
        HelixSendMessageArgs args,
        ResultCallback<HelixSentMessage> successCallback,
        FailureCallback<HelixSendMessageError, QString> failureCallback) final;

    /// https://dev.twitch.tv/docs/api/reference/#get-user-emotes
    void getUserEmotes(
        QString userID, QString broadcasterID,
        ResultCallback<std::vector<HelixChannelEmote>, HelixPaginationState>
            pageCallback,
        FailureCallback<QString> failureCallback,
        CancellationToken &&token) final;

    /// https://dev.twitch.tv/docs/api/reference/#get-followed-channels
    /// (non paginated)
    void getFollowedChannel(
        QString userID, QString broadcasterID, const QObject *caller,
        ResultCallback<std::optional<HelixFollowedChannel>> successCallback,
        FailureCallback<QString> failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#create-eventsub-subscription
    void createEventSubSubscription(
        const eventsub::SubscriptionRequest &request, const QString &sessionID,
        ResultCallback<HelixCreateEventSubSubscriptionResponse> successCallback,
        FailureCallback<HelixCreateEventSubSubscriptionError, QString>
            failureCallback) final;

    // https://dev.twitch.tv/docs/api/reference/#delete-eventsub-subscription
    void deleteEventSubSubscription(
        const QString &subscriptionID, ResultCallback<> successCallback,
        FailureCallback<QString> failureCallback) final;

    void update(QString clientId, QString oauthToken) final;

    static void initialize();

protected:
    // https://dev.twitch.tv/docs/api/reference#update-chat-settings
    void updateChatSettings(
        QString broadcasterID, QString moderatorID, QJsonObject json,
        ResultCallback<HelixChatSettings> successCallback,
        FailureCallback<HelixUpdateChatSettingsError, QString> failureCallback)
        final;

    // Recursive boy
    void onFetchChattersSuccess(
        std::shared_ptr<HelixChatters> finalChatters, QString broadcasterID,
        QString moderatorID, size_t maxChattersToFetch,
        ResultCallback<HelixChatters> successCallback,
        FailureCallback<HelixGetChattersError, QString> failureCallback,
        HelixChatters chatters);

    // Get chatters list - This method is what actually runs the API request
    // https://dev.twitch.tv/docs/api/reference#get-chatters
    void fetchChatters(
        QString broadcasterID, QString moderatorID, int first, QString after,
        ResultCallback<HelixChatters> successCallback,
        FailureCallback<HelixGetChattersError, QString> failureCallback);

    // Recursive boy
    void onFetchModeratorsSuccess(
        std::shared_ptr<std::vector<HelixModerator>> finalModerators,
        QString broadcasterID, size_t maxModeratorsToFetch,
        ResultCallback<std::vector<HelixModerator>> successCallback,
        FailureCallback<HelixGetModeratorsError, QString> failureCallback,
        HelixModerators moderators);

    // Get moderator list - This method is what actually runs the API request
    // https://dev.twitch.tv/docs/api/reference#get-moderators
    void fetchModerators(
        QString broadcasterID, int first, QString after,
        ResultCallback<HelixModerators> successCallback,
        FailureCallback<HelixGetModeratorsError, QString> failureCallback);

private:
    NetworkRequest makeRequest(const QString &url, const QUrlQuery &urlQuery,
                               NetworkRequestType type);
    NetworkRequest makeGet(const QString &url, const QUrlQuery &urlQuery);
    NetworkRequest makeDelete(const QString &url, const QUrlQuery &urlQuery);
    NetworkRequest makePost(const QString &url, const QUrlQuery &urlQuery);
    NetworkRequest makePut(const QString &url, const QUrlQuery &urlQuery);
    NetworkRequest makePatch(const QString &url, const QUrlQuery &urlQuery);

    /// Paginate the `url` endpoint and use `baseQuery` as the starting point for pagination.
    /// @param onPage returns true while a new page is expected. Once false is returned, pagination will stop.
    void paginate(const QString &url, const QUrlQuery &baseQuery,
                  std::function<bool(const QJsonObject &,
                                     const HelixPaginationState &state)>
                      onPage,
                  std::function<void(NetworkResult)> onError,
                  CancellationToken &&token);

    QString clientId;
    QString oauthToken;
};

// initializeHelix sets the helix instance to _instance
// from a normal application, this should never be called, and will instead be handled by calling Helix::initialize()
void initializeHelix(IHelix *_instance);

IHelix *getHelix();

}  // namespace chatterino
