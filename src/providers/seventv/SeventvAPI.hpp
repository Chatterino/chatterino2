#pragma once

#include <functional>

class QString;
class QJsonObject;

namespace chatterino {

class NetworkResult;

class SeventvAPI
{
    using ErrorCallback = std::function<void(const NetworkResult &)>;
    template <typename... T>
    using SuccessCallback = std::function<void(T...)>;

public:
    void getUserByTwitchID(const QString &twitchID,
                           SuccessCallback<const QJsonObject &> &&onSuccess,
                           ErrorCallback &&onError);
    void getEmoteSet(const QString &emoteSet,
                     SuccessCallback<const QJsonObject &> &&onSuccess,
                     ErrorCallback &&onError);

    void updatePresence(const QString &twitchChannelID,
                        const QString &seventvUserID,
                        SuccessCallback<> &&onSuccess, ErrorCallback &&onError);
};

SeventvAPI &getSeventvAPI();

}  // namespace chatterino
