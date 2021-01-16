#pragma once

#include "common/NetworkRequest.hpp"

#include <QString>
#include <QStringList>
#include <QUrlQuery>

#include <functional>

namespace chatterino {

using KrakenFailureCallback = std::function<void()>;
template <typename... T>
using ResultCallback = std::function<void(T...)>;

struct KrakenChannel {
    const QString status;

    KrakenChannel(QJsonObject jsonObject)
        : status(jsonObject.value("status").toString())
    {
    }
};

class Kraken final : boost::noncopyable
{
public:
    // https://dev.twitch.tv/docs/v5/reference/users#follow-channel
    void getChannel(QString userId,
                    ResultCallback<KrakenChannel> resultCallback,
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
