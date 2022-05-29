#pragma once

#include <QColor>
#include <QString>

namespace chatterino {

#ifndef ATTR_UNUSED
#    ifdef Q_OS_WIN
#        define ATTR_UNUSED
#    else
#        define ATTR_UNUSED __attribute__((unused))
#    endif
#endif

static const char *ANONYMOUS_USERNAME ATTR_UNUSED = "justinfan64537";

inline QByteArray getDefaultClientID()
{
    return QByteArray("7ue61iz46fz11y3cugd0l3tawb4taal");
}

static const std::vector<QColor> TWITCH_USERNAME_COLORS = {
    {255, 0, 0},      // Red
    {0, 0, 255},      // Blue
    {0, 255, 0},      // Green
    {178, 34, 34},    // FireBrick
    {255, 127, 80},   // Coral
    {154, 205, 50},   // YellowGreen
    {255, 69, 0},     // OrangeRed
    {46, 139, 87},    // SeaGreen
    {218, 165, 32},   // GoldenRod
    {210, 105, 30},   // Chocolate
    {95, 158, 160},   // CadetBlue
    {30, 144, 255},   // DodgerBlue
    {255, 105, 180},  // HotPink
    {138, 43, 226},   // BlueViolet
    {0, 255, 127},    // SpringGreen
};

static const QStringList TWITCH_DEFAULT_COMMANDS{
    "help",
    "w",
    "me",
    "disconnect",
    "mods",
    "vips",
    "color",
    "commercial",
    "mod",
    "unmod",
    "vip",
    "unvip",
    "ban",
    "unban",
    "timeout",
    "untimeout",
    "slow",
    "slowoff",
    "r9kbeta",
    "r9kbetaoff",
    "emoteonly",
    "emoteonlyoff",
    "clear",
    "subscribers",
    "subscribersoff",
    "followers",
    "followersoff",
    "host",
    "unhost",
    "raid",
    "unraid",
    "delete",
    "announce",
    "requests",
};

static const QStringList TWITCH_WHISPER_COMMANDS{"/w", ".w"};

}  // namespace chatterino
