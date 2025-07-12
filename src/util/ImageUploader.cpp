#include "util/ImageUploader.hpp"

#include "singletons/Settings.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace chatterino::imageuploader::detail {

namespace {

QStringList parseHeaders(const QJsonObject &headersObj)
{
    QStringList headerLines;
    for (auto it = headersObj.begin(); it != headersObj.end(); ++it)
    {
        if (it.value().isString())
        {
            headerLines.append(
                QString("%1: %2").arg(it.key(), it.value().toString()));
        }
    }
    return headerLines;
}

}  // namespace

QJsonObject exportSettings(const Settings &s)
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
    if (!headers.isEmpty())
    {
        QJsonObject headersObj;
        QStringList headerLines = headers.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : headerLines)
        {
            QStringList parts = line.split(':', Qt::SkipEmptyParts);
            if (parts.size() >= 2)
            {
                QString key = parts[0].trimmed();
                QString value = parts.mid(1).join(':').trimmed();
                headersObj[key] = value;
            }
        }
        if (!headersObj.isEmpty())
        {
            settingsObj["Headers"] = headersObj;
        }
    }

    return settingsObj;
}

bool importSettings(const QJsonObject &settingsObj, Settings &s)
{
    if (!settingsObj.contains("RequestURL") ||
        !settingsObj["RequestURL"].isString() ||
        !settingsObj.contains("FileFormName") ||
        !settingsObj["FileFormName"].isString() ||
        !settingsObj.contains("URL") || !settingsObj["URL"].isString())
    {
        return false;
    }

    s.imageUploaderUrl = settingsObj["RequestURL"].toString();
    s.imageUploaderFormField = settingsObj["FileFormName"].toString();
    s.imageUploaderLink = settingsObj["URL"].toString();

    if (settingsObj.contains("DeletionURL") &&
        settingsObj["DeletionURL"].isString())
    {
        s.imageUploaderDeletionLink = settingsObj["DeletionURL"].toString();
    }

    if (settingsObj.contains("Headers") && settingsObj["Headers"].isObject())
    {
        QStringList headers = parseHeaders(settingsObj["Headers"].toObject());
        if (!headers.isEmpty())
        {
            s.imageUploaderHeaders = headers.join('\n');
        }
    }

    s.imageUploaderEnabled = true;

    return true;
}

ExpectedStr<QJsonObject> validateImportJson(const QString &clipboardText)
{
    if (clipboardText.isEmpty())
    {
        return nonstd::make_unexpected("Clipboard must not be empty");
    }

    QJsonParseError parseError;
    QJsonDocument doc =
        QJsonDocument::fromJson(clipboardText.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        return nonstd::make_unexpected("Clipboard did not contain valid JSON");
    }

    if (!doc.isObject())
    {
        return nonstd::make_unexpected("JSON must be an object");
    }

    auto settingsObj = doc.object();

    if (!settingsObj.contains("Version"))
    {
        return nonstd::make_unexpected("JSON must contain the 'Version' key");
    }

    if (!settingsObj.contains("Name"))
    {
        return nonstd::make_unexpected("JSON must contain the 'Name' key");
    }

    return settingsObj;
}

}  // namespace chatterino::imageuploader::detail
