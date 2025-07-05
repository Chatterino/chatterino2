#pragma once

class QJsonObject;

namespace chatterino {

class Settings;

namespace imageuploader {
namespace detail {

QJsonObject exportSettings(Settings &s);
bool importSettings(const QJsonObject &settingsObj, Settings &s);
bool validateImportJson(const QString &clipboardText, QJsonObject &settingsObj);
void parseAndApplyHeaders(const QJsonObject &headersObj, Settings &s);

}  // namespace detail
}  // namespace imageuploader

}  // namespace chatterino
