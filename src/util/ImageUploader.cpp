#include "util/ImageUploader.hpp"

#include "singletons/Settings.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace chatterino {
namespace imageuploader {
namespace detail {

QJsonObject exportSettings(Settings &s)
{
    QJsonObject settingsObj;
    settingsObj["Version"] = "1.0.0";
    settingsObj["Name"] = "Chatterino Image Uploader Settings";
    settingsObj["RequestMethod"] = "POST";
    settingsObj["RequestURL"] = s.imageUploaderUrl.getValue();
    settingsObj["Body"] = "MultipartFormData";
    settingsObj["FileFormName"] = s.imageUploaderFormField.getValue();
    settingsObj["URL"] = s.imageUploaderLink.getValue();
    settingsObj["DeletionURL"] = s.imageUploaderDeletionLink.getValue();

    QString headers = s.imageUploaderHeaders.getValue();
    if (!headers.isEmpty()) {
        QJsonObject headersObj;
        QStringList headerLines = headers.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : headerLines) {
            QStringList parts = line.split(':', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString key = parts[0].trimmed();
                QString value = parts.mid(1).join(':').trimmed();
                headersObj[key] = value;
            }
        }
        if (!headersObj.isEmpty()) {
            settingsObj["Headers"] = headersObj;
        }
    }

    return settingsObj;
}

bool importSettings(const QJsonObject &settingsObj, Settings &s)
{
    bool hasValidSettings = false;

    if (settingsObj.contains("RequestURL") && settingsObj["RequestURL"].isString()) {
        s.imageUploaderUrl = settingsObj["RequestURL"].toString();
        hasValidSettings = true;
    }

    if (settingsObj.contains("FileFormName") && settingsObj["FileFormName"].isString()) {
        s.imageUploaderFormField = settingsObj["FileFormName"].toString();
        hasValidSettings = true;
    }

    if (settingsObj.contains("URL") && settingsObj["URL"].isString()) {
        s.imageUploaderLink = settingsObj["URL"].toString();
        hasValidSettings = true;
    }

    if (settingsObj.contains("DeletionURL") && settingsObj["DeletionURL"].isString()) {
        s.imageUploaderDeletionLink = settingsObj["DeletionURL"].toString();
        hasValidSettings = true;
    }

    if (settingsObj.contains("Headers") && settingsObj["Headers"].isObject()) {
        parseAndApplyHeaders(settingsObj["Headers"].toObject(), s);
        hasValidSettings = true;
    }

    if (hasValidSettings) {
        s.imageUploaderEnabled = true;
    }

    return hasValidSettings;
}

bool validateImportJson(const QString &clipboardText, QJsonObject &settingsObj)
{
    if (clipboardText.isEmpty()) {
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(clipboardText.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isObject()) {
        return false;
    }

    settingsObj = doc.object();
    return true;
}

void parseAndApplyHeaders(const QJsonObject &headersObj, Settings &s)
{
    QStringList headerLines;

    for (auto it = headersObj.begin(); it != headersObj.end(); ++it) {
        if (it.value().isString()) {
            headerLines.append(QString("%1: %2").arg(it.key(), it.value().toString()));
        }
    }

    if (!headerLines.isEmpty()) {
        s.imageUploaderHeaders = headerLines.join('\n');
    }
}

}  // namespace detail
}  // namespace imageuploader
}  // namespace chatterino
