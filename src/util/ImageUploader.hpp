#pragma once

class QJsonObject;
class QString;

namespace chatterino {

class Settings;

namespace imageuploader {
namespace detail {

// Exports current image uploader settings to JSON format.
QJsonObject exportSettings(const Settings &s);

// Imports image uploader settings from JSON into the Settings object.
bool importSettings(const QJsonObject &settingsObj, Settings &s);

// Validates if the clipboard text contains valid JSON and parses it.
bool validateImportJson(const QString &clipboardText, QJsonObject &settingsObj);

}  // namespace detail
}  // namespace imageuploader

}  // namespace chatterino
