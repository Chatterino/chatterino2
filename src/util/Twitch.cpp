#include "util/Twitch.hpp"

#include <QDesktopServices>
#include <QUrl>

namespace chatterino {

void openTwitchUsercard(QString channel, QString username)
{
    QDesktopServices::openUrl("https://www.twitch.tv/popout/" + channel +
                              "/viewercard/" + username);
}

void stripUserName(QString &userName)
{
    if (userName.startsWith('@'))
    {
        userName.remove(0, 1);
    }
    if (userName.endsWith(','))
    {
        userName.chop(1);
    }
}

void stripChannelName(QString &channelName)
{
    if (channelName.startsWith('@') || channelName.startsWith('#'))
    {
        channelName.remove(0, 1);
    }
    if (channelName.endsWith(','))
    {
        channelName.chop(1);
    }
}

QRegularExpression twitchUsernameRegexp()
{
    static QRegularExpression twitchUsernameRegexp(
        "^[a-zA-Z0-9][a-zA-Z0-9_]{0,24}$");

    return twitchUsernameRegexp;
}

}  // namespace chatterino
