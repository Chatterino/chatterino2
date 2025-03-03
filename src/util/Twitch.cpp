#include "util/Twitch.hpp"

#include "util/QStringHash.hpp"

#include <QDesktopServices>
#include <QUrl>

#include <unordered_map>

namespace chatterino {

namespace {

    const auto TWITCH_USER_LOGIN_PATTERN = R"(^[a-z0-9]\w{0,24}$)";

    // Remember to keep VALID_HELIX_COLORS up-to-date if a new color is implemented to keep naming for users consistent
    const std::unordered_map<QString, QString> HELIX_COLOR_REPLACEMENTS{
        {"blueviolet", "blue_violet"},   {"cadetblue", "cadet_blue"},
        {"dodgerblue", "dodger_blue"},   {"goldenrod", "golden_rod"},
        {"hotpink", "hot_pink"},         {"orangered", "orange_red"},
        {"seagreen", "sea_green"},       {"springgreen", "spring_green"},
        {"yellowgreen", "yellow_green"},
    };

}  // namespace

// Colors retreived from https://dev.twitch.tv/docs/api/reference#update-user-chat-color 2022-09-11
// Remember to keep HELIX_COLOR_REPLACEMENTS up-to-date if a new color is implemented to keep naming for users consistent
extern const QStringList VALID_HELIX_COLORS{
    "blue",        "blue_violet", "cadet_blue", "chocolate",    "coral",
    "dodger_blue", "firebrick",   "golden_rod", "green",        "hot_pink",
    "orange_red",  "red",         "sea_green",  "spring_green", "yellow_green",
};

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

QString cleanChannelName(const QString &dirtyChannelName)
{
    if (dirtyChannelName.startsWith('#'))
    {
        return dirtyChannelName.mid(1).toLower();
    }

    return dirtyChannelName.toLower();
}

std::pair<ParsedUserName, ParsedUserID> parseUserNameOrID(const QString &input)
{
    if (input.startsWith("id:"))
    {
        return {
            {},
            input.mid(3),
        };
    }

    QString userName = input;

    if (userName.startsWith('@') || userName.startsWith('#'))
    {
        userName.remove(0, 1);
    }
    if (userName.endsWith(','))
    {
        userName.chop(1);
    }

    return {
        userName,
        {},
    };
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
