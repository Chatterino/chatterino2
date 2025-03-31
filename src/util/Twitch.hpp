#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace chatterino {

extern const QStringList VALID_HELIX_COLORS;

void openTwitchUsercard(const QString channel, const QString username);

// stripUserName removes any @ prefix or , suffix to make it more suitable for command use
void stripUserName(QString &userName);

// stripChannelName removes any @ prefix or , suffix to make it more suitable for command use
void stripChannelName(QString &channelName);

/// Strips a leading `#` and lowercases the name
QString cleanChannelName(const QString &dirtyChannelName);

using ParsedUserName = QString;
using ParsedUserID = QString;

/**
 * Parse the given input into either a user name or a user ID
 *
 * User IDs take priority and are parsed if the input starts with `id:`
 */
std::pair<ParsedUserName, ParsedUserID> parseUserNameOrID(const QString &input);

// Matches a strict Twitch user login.
// May contain lowercase a-z, 0-9, and underscores
// Must contain between 1 and 25 characters
// Must not start with an underscore
QRegularExpression twitchUserLoginRegexp();

// Matches a loose Twitch user login name.
// May contain lowercase and uppercase a-z, 0-9, and underscores
// Must contain between 1 and 25 characters
// Must not start with an underscore
QRegularExpression twitchUserNameRegexp();

// Cleans up a color name input for use in the Helix API
// Will help massage color names like BlueViolet to the helix-acceptible blue_violet
// Will also lowercase the color
void cleanHelixColorName(QString &color);

}  // namespace chatterino
