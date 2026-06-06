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

/// Gets the "type" key from the given object and compares its value with expectedType
bool matchesType(const rapidjson::Value &obj, QStringView expectedType);

/// Gets the "id" key from the given object and compares its value with expectedID
bool matchesID(const rapidjson::Value &obj, QStringView expectedID);

/// Generate a random ID (UUIDv4) for a new highlight
QString generateID();

QStringView getID(const AllHighlights &h);

QString getDefaultName(const AllHighlights &h);

QString getName(const AllHighlights &h);

bool isEnabled(const AllHighlights &h);

QUrl getSoundURL(const AllHighlights &h);

bool shouldShowInMentions(const AllHighlights &h);

bool shouldAlert(const AllHighlights &h);

bool shouldPlaySound(const AllHighlights &h);

bool willPlayCustomSound(const AllHighlights &h);

QIcon getIcon(const AllHighlights &h);

/// Get the background color defined for the highlight, or its default value
std::shared_ptr<QColor> getBackgroundColor(const AllHighlights &h);

}  // namespace chatterino::highlights
