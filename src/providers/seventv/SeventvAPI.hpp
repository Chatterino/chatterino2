#pragma once

#include <functional>

class QString;
class QJsonObject;

namespace chatterino {

class NetworkResult;

class SeventvAPI final
{
    using ErrorCallback = std::function<void(const NetworkResult &)>;
    template <typename... T>
    using SuccessCallback = std::function<void(T...)>;

public:
    SeventvAPI() = default;
    ~SeventvAPI() = default;

    SeventvAPI(const SeventvAPI &) = delete;
    SeventvAPI(SeventvAPI &&) = delete;
    SeventvAPI &operator=(const SeventvAPI &) = delete;
    SeventvAPI &operator=(SeventvAPI &&) = delete;

    virtual void getUserByTwitchID(
        const QString &twitchID,
        SuccessCallback<const QJsonObject &> &&onSuccess,
        ErrorCallback &&onError);
    virtual void getEmoteSet(const QString &emoteSet,
                             SuccessCallback<const QJsonObject &> &&onSuccess,
                             ErrorCallback &&onError);

    virtual void updatePresence(const QString &twitchChannelID,
                                const QString &seventvUserID,
                                SuccessCallback<> &&onSuccess,
                                ErrorCallback &&onError);
};

}  // namespace chatterino
