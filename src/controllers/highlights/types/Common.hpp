#pragma once

#include <QString>
#include <QStringView>
#include <rapidjson/document.h>

namespace chatterino::highlights {

/// Gets the "type" key from the given object and compares its value with expectedType
bool matchesType(const rapidjson::Value &obj, QStringView expectedType);

/// Gets the "id" key from the given object and compares its value with expectedID
bool matchesID(const rapidjson::Value &obj, QStringView expectedID);

/// Generate a random ID (UUIDv4) for a new highlight
QString generateID();

}  // namespace chatterino::highlights
