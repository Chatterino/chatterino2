#include "providers/twitch/TwitchCommon.hpp"

namespace chatterino {

const std::vector<QColor> TWITCH_USERNAME_COLORS = {
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

const QStringList TWITCH_DEFAULT_COMMANDS{
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
    "warn",
};

const QStringList TWITCH_WHISPER_COMMANDS{"/w", ".w"};

}  // namespace chatterino
