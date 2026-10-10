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

QStringView getID(const AnyHighlight &h);

QString getDefaultName(const AnyHighlight &h);

/// Returns the highlight type name (i.e. "Message highlight" for the MessageHighlight type);
QStringView getHighlightTypeName(const AnyHighlight &h);

QString getName(const AnyHighlight &h);

bool isEnabled(const AnyHighlight &h);

/// Get the configured sound string - same as stored in the settings.
/// If a default sound is enabled, and no string is configured, it returns the default sound string.
QString getSound(const AnyHighlight &h);

/// Gets the raw string the user has configured as the sound for the highlight.
QString getSoundWithoutDefault(const AnyHighlight &h);

/// Returns the default sound of the highlight, as defined by the `SOUND_DEFAULT` static string.
QStringView getDefaultSound(const AnyHighlight &h);

/// Gets the resolved URL for the sound that should play when this highlight is triggered.
/// This takes the default sound of the highlight into consideration if the user has not configured any sound.
QUrl getSoundURL(const AnyHighlight &h);

bool shouldShowInMentions(const AnyHighlight &h);

bool shouldAlert(const AnyHighlight &h);

bool shouldPlaySound(const AnyHighlight &h);

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
