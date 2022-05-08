#include "util/Twitch.hpp"

#include <QDesktopServices>
#include <QUrl>

namespace chatterino {

namespace {

    const auto TWITCH_USER_LOGIN_PATTERN = R"(^[a-z0-9]\w{0,24}$)";

}  // namespace

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

QRegularExpression twitchUserNameRegexp()
{
    static QRegularExpression re(
        TWITCH_USER_LOGIN_PATTERN,
        QRegularExpression::PatternOption::CaseInsensitiveOption);

    return re;
}

QRegularExpression twitchUserLoginRegexp()
{
    static QRegularExpression re(TWITCH_USER_LOGIN_PATTERN);

    return re;
}

}  // namespace chatterino
