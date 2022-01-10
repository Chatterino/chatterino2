#pragma once

#include "common/Aliases.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchEmotes.hpp"

#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <vector>

namespace chatterino {

using HelixFailureCallback = std::function<void()>;
template <typename... T>
using ResultCallback = std::function<void(T...)>;

struct HelixUser {
    QString id;
    QString login;
    QString displayName;
    QString createdAt;
    QString description;
    QString profileImageUrl;
    int viewCount;

    explicit HelixUser(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , login(jsonObject.value("login").toString())
        , displayName(jsonObject.value("display_name").toString())
        , createdAt(jsonObject.value("created_at").toString())
        , description(jsonObject.value("description").toString())
        , profileImageUrl(jsonObject.value("profile_image_url").toString())
        , viewCount(jsonObject.value("view_count").toInt())
    {
    }
};

struct HelixUsersFollowsRecord {
    QString fromId;
    QString fromName;
    QString toId;
    QString toName;
    QString followedAt;  // date time object

    HelixUsersFollowsRecord()
        : fromId("")
        , fromName("")
        , toId("")
        , toName("")
        , followedAt("")
    {
    }

    explicit HelixUsersFollowsRecord(QJsonObject jsonObject)
        : fromId(jsonObject.value("from_id").toString())
        , fromName(jsonObject.value("from_name").toString())
        , toId(jsonObject.value("to_id").toString())
        , toName(jsonObject.value("to_name").toString())
        , followedAt(jsonObject.value("followed_at").toString())
    {
    }
};

struct HelixUsersFollowsResponse {
    int total;
    std::vector<HelixUsersFollowsRecord> data;
    explicit HelixUsersFollowsResponse(QJsonObject jsonObject)
        : total(jsonObject.value("total").toInt())
    {
        const auto &jsonData = jsonObject.value("data").toArray();
        std::transform(jsonData.begin(), jsonData.end(),
                       std::back_inserter(this->data),
                       [](const QJsonValue &record) {
                           return HelixUsersFollowsRecord(record.toObject());
                       });
    }
};

struct HelixStream {
    QString id;  // stream id
    QString userId;
    QString userName;
    QString gameId;
    QString type;
    QString title;
    int viewerCount;
    QString startedAt;
    QString language;
    QString thumbnailUrl;

    HelixStream()
        : id("")
        , userId("")
        , userName("")
        , gameId("")
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
        , userName(jsonObject.value("user_name").toString())
        , gameId(jsonObject.value("game_id").toString())
        , type(jsonObject.value("type").toString())
        , title(jsonObject.value("title").toString())
        , viewerCount(jsonObject.value("viewer_count").toInt())
        , startedAt(jsonObject.value("started_at").toString())
        , language(jsonObject.value("language").toString())
        , thumbnailUrl(jsonObject.value("thumbnail_url").toString())
    {
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
    const QString emoteId;
    const QString name;
    const QString type;
    const QString setId;
    const QString url;

    explicit HelixChannelEmote(QJsonObject jsonObject)
        : emoteId(jsonObject.value("id").toString())
        , name(jsonObject.value("name").toString())
        , type(jsonObject.value("emote_type").toString())
        , setId(jsonObject.value("emote_set_id").toString())
        , url(QString(TWITCH_EMOTE_TEMPLATE)
                  .replace("{id}", this->emoteId)
                  .replace("{scale}", "3.0"))
    {
    }
};

enum class HelixClipError {
    Unknown,
    ClipsDisabled,
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

class Helix final : boost::noncopyable
{
public:
    // https://dev.twitch.tv/docs/api/reference#get-users
    void fetchUsers(QStringList userIds, QStringList userLogins,
                    ResultCallback<std::vector<HelixUser>> successCallback,
                    HelixFailureCallback failureCallback);
    void getUserByName(QString userName,
                       ResultCallback<HelixUser> successCallback,
                       HelixFailureCallback failureCallback);
    void getUserById(QString userId, ResultCallback<HelixUser> successCallback,
                     HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#get-users-follows
    void fetchUsersFollows(
        QString fromId, QString toId,
        ResultCallback<HelixUsersFollowsResponse> successCallback,
        HelixFailureCallback failureCallback);

    void getUserFollowers(
        QString userId,
        ResultCallback<HelixUsersFollowsResponse> successCallback,
        HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#get-streams
    void fetchStreams(QStringList userIds, QStringList userLogins,
                      ResultCallback<std::vector<HelixStream>> successCallback,
                      HelixFailureCallback failureCallback);

    void getStreamById(QString userId,
                       ResultCallback<bool, HelixStream> successCallback,
                       HelixFailureCallback failureCallback);

    void getStreamByName(QString userName,
                         ResultCallback<bool, HelixStream> successCallback,
                         HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#get-games
    void fetchGames(QStringList gameIds, QStringList gameNames,
                    ResultCallback<std::vector<HelixGame>> successCallback,
                    HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#search-categories
    void searchGames(QString gameName,
                     ResultCallback<std::vector<HelixGame>> successCallback,
                     HelixFailureCallback failureCallback);

    void getGameById(QString gameId, ResultCallback<HelixGame> successCallback,
                     HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#create-clip
    void createClip(QString channelId,
                    ResultCallback<HelixClip> successCallback,
                    std::function<void(HelixClipError)> failureCallback,
                    std::function<void()> finallyCallback);

    // https://dev.twitch.tv/docs/api/reference#get-channel-information
    void getChannel(QString broadcasterId,
                    ResultCallback<HelixChannel> successCallback,
                    HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference/#create-stream-marker
    void createStreamMarker(
        QString broadcasterId, QString description,
        ResultCallback<HelixStreamMarker> successCallback,
        std::function<void(HelixStreamMarkerError)> failureCallback);

    // https://dev.twitch.tv/docs/api/reference#get-user-block-list
    void loadBlocks(QString userId,
                    ResultCallback<std::vector<HelixBlock>> successCallback,
                    HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#block-user
    void blockUser(QString targetUserId, std::function<void()> successCallback,
                   HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#unblock-user
    void unblockUser(QString targetUserId,
                     std::function<void()> successCallback,
                     HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#modify-channel-information
    void updateChannel(QString broadcasterId, QString gameId, QString language,
                       QString title,
                       std::function<void(NetworkResult)> successCallback,
                       HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages
    void manageAutoModMessages(
        QString userID, QString msgID, QString action,
        std::function<void()> successCallback,
        std::function<void(HelixAutoModMessageError)> failureCallback);

    // https://dev.twitch.tv/docs/api/reference/#get-cheermotes
    void getCheermotes(
        QString broadcasterId,
        ResultCallback<std::vector<HelixCheermoteSet>> successCallback,
        HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#get-emote-sets
    void getEmoteSetData(QString emoteSetId,
                         ResultCallback<HelixEmoteSetData> successCallback,
                         HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#get-channel-emotes
    void getChannelEmotes(
        QString broadcasterId,
        ResultCallback<std::vector<HelixChannelEmote>> successCallback,
        HelixFailureCallback failureCallback);

    void update(QString clientId, QString oauthToken);

    static void initialize();

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);

    QString clientId;
    QString oauthToken;
};

Helix *getHelix();

}  // namespace chatterino
