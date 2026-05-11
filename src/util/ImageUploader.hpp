// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/Expected.hpp"

class QJsonObject;
class QString;

namespace chatterino {

class Settings;

namespace imageuploader::detail {

// Exports current image uploader settings to JSON format.
QJsonObject exportSettings(const Settings &s);

// Imports image uploader settings from JSON into the Settings object.
bool importSettings(const QJsonObject &settingsObj, Settings &s);

// Validates if the clipboard text contains valid JSON and parses it.
ExpectedStr<QJsonObject> validateImportJson(const QString &clipboardText);

}  // namespace imageuploader::detail

}  // namespace chatterino
