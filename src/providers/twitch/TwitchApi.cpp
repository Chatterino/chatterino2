#include "providers/twitch/TwitchApi.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchCommon.hpp"

#include <QString>
#include <QThread>

namespace chatterino {

void TwitchApi::findUserId(const QString user,
                           std::function<void(QString)> successCallback)
{
    QString requestUrl("https://api.twitch.tv/kraken/users?login=" + user);

    NetworkRequest request(requestUrl);
    request.setCaller(QThread::currentThread());
    request.makeAuthorizedV5(getDefaultClientID());
    request.setTimeout(30000);
    request.onSuccess([successCallback](auto result) mutable -> Outcome {
        auto root = result.parseJson();
        if (!root.value("users").isArray()) {
            log("API Error while getting user id, users is not an array");
            successCallback("");
            return Failure;
        }
        auto users = root.value("users").toArray();
        if (users.size() != 1) {
            log("API Error while getting user id, users array size is not 1");
            successCallback("");
            return Failure;
        }
        if (!users[0].isObject()) {
            log("API Error while getting user id, first user is not an object");
            successCallback("");
            return Failure;
        }
        auto firstUser = users[0].toObject();
        auto id = firstUser.value("_id");
        if (!id.isString()) {
            log("API Error: while getting user id, first user object `_id` key "
                "is not a "
                "string");
            successCallback("");
            return Failure;
        }
        successCallback(id.toString());
        return Success;
    });

    request.execute();
}

}  // namespace chatterino
