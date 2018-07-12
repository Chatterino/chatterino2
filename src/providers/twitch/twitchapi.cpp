#include "providers/twitch/twitchapi.hpp"

#include "Application.hpp"
#include "common/UrlFetch.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchCommon.hpp"

#include <QString>
#include <QThread>

namespace chatterino {

void TwitchApi::FindUserId(const QString user, std::function<void(QString)> callback)
{
    QString requestUrl("https://api.twitch.tv/kraken/users?login=" + user +
                       "&api_version=5&client_id=" + getDefaultClientID());
    NetworkRequest request(requestUrl);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([callback](auto result) mutable {
        QString userId;

        auto root = result.parseJson();
        if (root.value("users").toArray().isEmpty()) {
            callback("");
            return true;
        }
        userId = root.value("users").toArray()[0].toObject().value("_id").toString();
        callback(userId);
        return true;
    });

    request.execute();
    return;
}

}  // namespace chatterino
