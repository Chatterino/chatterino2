// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/AllForward.hpp"

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

QStringView getID(const AllHighlights &h);

QString getDefaultName(const AllHighlights &h);

QString getName(const AllHighlights &h);

bool isEnabled(const AllHighlights &h);

/// Get the configured sound string - same as stored in the settings.
/// If a default sound is enabled, and no string is configured, it returns the default sound string.
QString getSound(const AllHighlights &h);

/// Gets the raw string the user has configured as the sound for the highlight.
QString getSoundWithoutDefault(const AllHighlights &h);

/// Returns the default sound of the highlight, as defined by the `SOUND_DEFAULT` static string.
QStringView getDefaultSound(const AllHighlights &h);

/// Gets the resolved URL for the sound that should play when this highlight is triggered.
/// This takes the default sound of the highlight into consideration if the user has not configured any sound.
QUrl getSoundURL(const AllHighlights &h);

bool shouldShowInMentions(const AllHighlights &h);

bool shouldAlert(const AllHighlights &h);

bool shouldPlaySound(const AllHighlights &h);

QIcon getIcon(const AllHighlights &h);

/// Get the background color defined for the highlight, or its default value
std::shared_ptr<QColor> getBackgroundColor(const AllHighlights &h);

/// Get the error of the highlight.
/// This could be an invalid filter or regular expression.
/// If no error, the returned QString will be empty.
/// If there's an error, the returned QString will return a message the user can read.
QString getError(const AllHighlights &h);

/// Returns true of the given highlight is user-defined
bool isUserDefined(const AllHighlights &h);

}  // namespace chatterino::highlights
