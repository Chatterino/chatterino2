#include "providers/twitch/PartialTwitchUser.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchCommon.hpp"

#include <QJsonArray>
#include <cassert>

namespace chatterino {

PartialTwitchUser PartialTwitchUser::byName(const QString &username)
{
    PartialTwitchUser user;
    user.username_ = username;

    return user;
}

PartialTwitchUser PartialTwitchUser::byId(const QString &id)
{
    PartialTwitchUser user;
    user.id_ = id;

    return user;
}

void PartialTwitchUser::getId(std::function<void(QString)> successCallback,
                              const QObject *caller)
{
    assert(!this->username_.isEmpty());

    NetworkRequest("https://api.twitch.tv/kraken/users?login=" +
                   this->username_)
        .caller(caller)
        .authorizeTwitchV5(getDefaultClientID())
        .onSuccess([successCallback](auto result) -> Outcome {
            auto root = result.parseJson();
            if (!root.value("users").isArray())
            {
                qDebug()
                    << "API Error while getting user id, users is not an array";
                return Failure;
            }

            auto users = root.value("users").toArray();
            if (users.size() != 1)
            {
                qDebug() << "API Error while getting user id, users array size "
                            "is not 1";
                return Failure;
            }
            if (!users[0].isObject())
            {
                qDebug() << "API Error while getting user id, first user is "
                            "not an object";
                return Failure;
            }
            auto firstUser = users[0].toObject();
            auto id = firstUser.value("_id");
            if (!id.isString())
            {
                qDebug() << "API Error: while getting user id, first user "
                            "object `_id` key is not a string";
                return Failure;
            }
            successCallback(id.toString());

            return Success;
        })
        .execute();
}

}  // namespace chatterino
