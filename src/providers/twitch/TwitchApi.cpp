#include "providers/twitch/TwitchApi.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/api/Helix.hpp"

#include <QString>
#include <QThread>

namespace chatterino {

void TwitchApi::findUserName(const QString userid,
                             std::function<void(QString)> successCallback)
{
    qDebug() << "DEPRECATED: Please stop using TwitchApi::findUserName";

    getHelix()->getUserById(
        userid,
        [successCallback](auto user) {
            successCallback(user.displayName);  //
        },
        [successCallback]() {
            successCallback("");  //
        });
}

}  // namespace chatterino
