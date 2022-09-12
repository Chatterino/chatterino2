#include "util/Twitch.hpp"

#include "util/QStringHash.hpp"

#include <QDesktopServices>
#include <QUrl>

#include <unordered_map>

namespace chatterino {

namespace {

    const auto TWITCH_USER_LOGIN_PATTERN = R"(^[a-z0-9]\w{0,24}$)";

    const std::unordered_map<QString, QString> HELIX_COLOR_REPLACEMENTS{
        {"blueviolet", "blue_violet"},   {"cadetblue", "cadet_blue"},
        {"dodgerblue", "dodger_blue"},   {"goldenrod", "golden_rod"},
        {"hotpink", "hot_pink"},         {"orangered", "orange_red"},
        {"seagreen", "sea_green"},       {"springgreen", "spring_green"},
        {"yellowgreen", "yellow_green"},
    };

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

void cleanHelixColorName(QString &color)
{
    color = color.toLower();
    auto it = HELIX_COLOR_REPLACEMENTS.find(color);

    if (it == HELIX_COLOR_REPLACEMENTS.end())
    {
        return;
    }

    color = it->second;
}

}  // namespace chatterino
