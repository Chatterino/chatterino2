#pragma once

#include "common/NetworkRequest.hpp"

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

enum class HelixClipError {
    Unknown,
    ClipsDisabled,
    UserNotAuthenticated,
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

    void getUserFollow(
        QString userId, QString targetId,
        ResultCallback<bool, HelixUsersFollowsRecord> successCallback,
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

    void getGameById(QString gameId, ResultCallback<HelixGame> successCallback,
                     HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#create-user-follows
    void followUser(QString userId, QString targetId,
                    std::function<void()> successCallback,
                    HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#delete-user-follows
    void unfollowUser(QString userId, QString targetlId,
                      std::function<void()> successCallback,
                      HelixFailureCallback failureCallback);

    // https://dev.twitch.tv/docs/api/reference#create-clip
    void createClip(QString channelId,
                    ResultCallback<HelixClip> successCallback,
                    std::function<void(HelixClipError)> failureCallback,
                    std::function<void()> finallyCallback);

    void update(QString clientId, QString oauthToken);

    static void initialize();

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);

    QString clientId;
    QString oauthToken;
};

Helix *getHelix();

}  // namespace chatterino
