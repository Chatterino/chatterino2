// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/AnyHighlightForward.hpp"

#include <QString>
#include <QStringView>
#include <rapidjson/document.h>

#include <memory>

class QUrl;
class QIcon;
class QColor;

namespace chatterino::highlights {

constexpr QStringView REGEX_START_BOUNDARY(u"(?:\\b|\\s|^)");
constexpr QStringView REGEX_END_BOUNDARY(u"(?:\\b|\\s|$)");

/// Returns true if the "type" key from the given `object` matches `expectedType`.
bool matchesType(const rapidjson::Value &object, QStringView expectedType);

/// Returns true if the "id" key from the given `object` matches `expectedID`.
bool matchesID(const rapidjson::Value &object, QStringView expectedID);

/// Generate a random ID (UUIDv4) for a new highlight
QString generateID();

/// Returns the ID of the highlight.
/// For built-in highlights, this ID describes the type of highlight this is (e.g. "announcements").
/// For user-defined highlights, this ID is a UUIDv4 for uniqueness.
QStringView getID(const AnyHighlight &h);

/// Returns the default name of the highlight.
QString getDefaultName(const AnyHighlight &h);

/// Returns the highlight type name (i.e. "Message highlight" for the MessageHighlight type);
QStringView getHighlightTypeName(const AnyHighlight &h);

/// Returns the user-defined name of the highlight, or, if no user-defined name is specified, its default name.
QString getName(const AnyHighlight &h);

/// Returns true if the highlight is enabled.
bool isEnabled(const AnyHighlight &h);

/// Get the configured sound string - same as stored in the settings.
/// If a default sound is enabled, and no string is configured, it returns the default sound string.
QString getSound(const AnyHighlight &h);

/// Returns the default sound of the highlight, as defined by the `SOUND_DEFAULT` static string.
QStringView getDefaultSound(const AnyHighlight &h);

/// Returns true if the message should show up in the /mentions channel if the highlight is triggered.
bool shouldShowInMentions(const AnyHighlight &h);

/// Returns true if the highlight should trigger a window alert.
bool shouldAlert(const AnyHighlight &h);

/// Returns true if the highlight will play any sound when triggered.
bool shouldPlaySound(const AnyHighlight &h);

/// Returns the icon for the highlight.
QIcon getIcon(const AnyHighlight &h);

/// Get the background color defined for the highlight, or its default value
std::shared_ptr<QColor> getBackgroundColor(const AnyHighlight &h);

/// Get the error of the highlight.
/// This could be an invalid filter or regular expression.
/// If no error, the returned QString will be empty.
/// If there's an error, the returned QString will return a message the user can read.
QString getError(const AnyHighlight &h);

/// Returns true of the given highlight is user-defined
bool isUserDefined(const AnyHighlight &h);

}  // namespace chatterino::highlights
