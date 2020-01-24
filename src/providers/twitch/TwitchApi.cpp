#include "providers/twitch/TwitchApi.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchCommon.hpp"

#include <QString>
#include <QThread>

namespace chatterino {

void TwitchApi::findUserId(const QString user,
                           std::function<void(QString)> successCallback)
{
    QString requestUrl("https://api.twitch.tv/kraken/users?login=" + user);

    NetworkRequest(requestUrl)

        .authorizeTwitchV5(getDefaultClientID())
        .timeout(30000)
        .onSuccess([successCallback](auto result) mutable -> Outcome {
            auto root = result.parseJson();
            if (!root.value("users").isArray())
            {
                qDebug()
                    << "API Error while getting user id, users is not an array";
                successCallback("");
                return Failure;
            }
            auto users = root.value("users").toArray();
            if (users.size() != 1)
            {
                qDebug() << "API Error while getting user id, users array size "
                            "is not 1";
                successCallback("");
                return Failure;
            }
            if (!users[0].isObject())
            {
                qDebug() << "API Error while getting user id, first user is "
                            "not an object";
                successCallback("");
                return Failure;
            }
            auto firstUser = users[0].toObject();
            auto id = firstUser.value("_id");
            if (!id.isString())
            {
                qDebug() << "API Error: while getting user id, first user "
                            "object `_id` key is not a string";
                successCallback("");
                return Failure;
            }
            successCallback(id.toString());
            return Success;
        })
        .execute();
}

void TwitchApi::findUserName(const QString userid,
                             std::function<void(QString)> successCallback)
{
    QString requestUrl("https://api.twitch.tv/kraken/users/" + userid);

    NetworkRequest(requestUrl)

        .authorizeTwitchV5(getDefaultClientID())
        .timeout(30000)
        .onSuccess([successCallback](auto result) mutable -> Outcome {
            auto root = result.parseJson();
            auto name = root.value("name");
            if (!name.isString())
            {
                qDebug() << "API Error: while getting user name, `name` is not "
                            "a string";
                successCallback("");
                return Failure;
            }
            successCallback(name.toString());
            return Success;
        })
        .execute();
}

}  // namespace chatterino
