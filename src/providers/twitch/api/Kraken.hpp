#pragma once

#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchAccount.hpp"

#include <QString>
#include <QStringList>
#include <QUrlQuery>

#include <functional>

namespace chatterino {

using KrakenFailureCallback = std::function<void()>;
template <typename... T>
using ResultCallback = std::function<void(T...)>;

struct KrakenEmoteSets {
    const QJsonObject emoteSets;

    KrakenEmoteSets(QJsonObject jsonObject)
        : emoteSets(jsonObject.value("emoticon_sets").toObject())
    {
    }
};

struct KrakenEmote {
    const QString code;
    const QString id;

    KrakenEmote(QJsonObject jsonObject)
        : code(jsonObject.value("code").toString())
        , id(QString::number(jsonObject.value("id").toInt()))
    {
    }
};

class Kraken final : boost::noncopyable
{
public:
    // https://dev.twitch.tv/docs/v5/reference/users#get-user-emotes
    void getUserEmotes(TwitchAccount *account,
                       ResultCallback<KrakenEmoteSets> successCallback,
                       KrakenFailureCallback failureCallback);

    void update(QString clientId, QString oauthToken);

    static void initialize();

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);

    QString clientId;
    QString oauthToken;
};

Kraken *getKraken();

}  // namespace chatterino
