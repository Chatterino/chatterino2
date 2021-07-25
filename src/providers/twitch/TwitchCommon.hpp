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

inline QString getDefaultClientID()
{
    // return "g5zg0400k4vhrx2g6xi4hgveruamlv"; // official one, used on chatterino.com/client_login
    return "t9xehoda0otdcfkzs24afh5h14wss6";  // zneix's testing instance
}

// Redirect URI used
inline QString getRedirectURI()
{
    return "http://localhost:52107/redirect";
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

static const QStringList loginScopes{
    // clang-format off
    "user_subscriptions",         //
    "user_blocks_edit",           // [DEPRECATED] replaced with "user:manage:blocked_users"
    "user_blocks_read",           // [DEPRECATED] replaced with "user:read:blocked_users"
    "user_follows_edit",          // [DEPRECATED] soon to be removed later since we now use "user:edit:follows"
    "channel_editor",             // [DEPRECATED] for /raid
    "channel:moderate",           //
    "channel:read:redemptions",   //
    "chat:edit",                  //
    "chat:read",                  //
    "whispers:read",              //
    "whispers:edit",              //
    "channel_commercial",         // [DEPRECATED] for /commercial
    "channel:edit:commercial",    // in case twitch upgrades things in the future (and this scope is required)
    "user:edit:follows",          // for (un)following
    "clips:edit",                 // for clip creation
    "channel:manage:broadcast",   // for creating stream markers with /marker command, and for the /settitle and /setgame commands
    "user:read:blocked_users",    // [DEPRECATED] for getting list of blocked users
    "user:manage:blocked_users",  // [DEPRECATED] for blocking/unblocking other users
    "moderator:manage:automod",   // for approving/denying automod messages
    // clang-format on
};

}  // namespace chatterino
