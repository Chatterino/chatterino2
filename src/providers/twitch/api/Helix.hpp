#pragma once

#include "common/NetworkRequest.hpp"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <functional>

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
        , viewCount(jsonObject.value("viewCount").toInt())
    {
    }
};

class Helix final : boost::noncopyable
{
public:
    void fetchUsers(QStringList userIds, QStringList userLogins,
                    ResultCallback<std::vector<HelixUser>> successCallback,
                    HelixFailureCallback failureCallback);
    void getUserByName(QString userName,
                       ResultCallback<HelixUser> successCallback,
                       HelixFailureCallback failureCallback);
    void getUserById(QString userId, ResultCallback<HelixUser> successCallback,
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
