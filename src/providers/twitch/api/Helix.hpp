#pragma once

#include "common/NetworkRequest.hpp"

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
template <typename T>
using ResultCallback = std::function<void(T)>;

struct HelixUser {
    const QString id;
    const QString login;
    const QString displayName;
    const QString description;
    const QString profileImageUrl;
    const int viewCount;

    HelixUser(QJsonObject jsonObject)
        : id(jsonObject.value("id").toString())
        , login(jsonObject.value("login").toString())
        , displayName(jsonObject.value("display_name").toString())
        , description(jsonObject.value("description").toString())
        , profileImageUrl(jsonObject.value("profile_image_url").toString())
        , viewCount(jsonObject.value("view_count").toInt())
    {
    }
};

struct HelixUsersFollowsRecord {
    const QString fromId;
    const QString fromName;
    const QString toId;
    const QString toName;
    const QString followedAt;  // date time object

    HelixUsersFollowsRecord(QJsonObject jsonObject)
        : fromId(jsonObject.value("from_id").toString())
        , fromName(jsonObject.value("from_name").toString())
        , toId(jsonObject.value("to_id").toString())
        , toName(jsonObject.value("to_name").toString())
        , followedAt(jsonObject.value("followed_at").toString())
    {
    }
};

struct HelixUsersFollowsResponse {
    const int total;
    std::vector<HelixUsersFollowsRecord> data;
    HelixUsersFollowsResponse(QJsonObject jsonObject)
        : total(jsonObject.value("total").toInt())
    {
        const auto &jsonData = jsonObject.value("data").toArray();
        std::transform(jsonData.begin(), jsonData.end(),
                       std::back_inserter(this->data),
                       [](const QJsonValue &asd) { return asd.toObject(); });
    }
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

    void update(QString clientId, QString oauthToken);

    static void initialize();
    static Helix *instance;

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);

    QString clientId;
    QString oauthToken;
};

Helix *getHelix();

}  // namespace chatterino
