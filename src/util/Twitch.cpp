#include "util/Twitch.hpp"

#include <QDesktopServices>
#include <QUrl>

namespace chatterino {

void openTwitchUsercard(QString channel, QString username)
{
    QDesktopServices::openUrl("https://www.twitch.tv/popout/" + channel +
                              "/viewercard/" + username);
}
}  // namespace chatterino
