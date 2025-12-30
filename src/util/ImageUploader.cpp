#include "util/ImageUploader.hpp"

#include "singletons/Settings.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace chatterino::imageuploader::detail {

namespace {

/**
  * @brief Converts ShareX-style tokens throughout the URL to the expected format.
  *
  * Examples:
  *  - https://example.com/{json:foo.bar[1]}/{json:foo.bar[2]}
  *     -> https://example.com/{foo.bar.1}/{foo.bar.2}
  *  - {json:{response}|foo.bar[1]}
  *     -> {foo.bar.1}
  *
  * @see https://github.com/ShareX/ShareX/blob/8c0fac3bdf6c6ecb9756584096332175f290072c/ShareX.UploadersLib/CustomUploader/ShareXSyntaxParser.cs
  * @see https://github.com/ShareX/ShareX/blob/8c0fac3bdf6c6ecb9756584096332175f290072c/ShareX.UploadersLib/CustomUploader/Functions/CustomUploaderFunctionJson.cs
  */
QString parseUrl(const QString &url)
{
    if (url.isEmpty() || url.compare("{response}", Qt::CaseInsensitive) == 0)
    {
        return {};
    }

    static const QRegularExpression tokenRegex(
        R"(\{json:([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}*)",
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression arrayRegex(R"(\[(\d+)\])");

    QString out;
    qsizetype last = 0;
    bool changed = false;

    auto it = tokenRegex.globalMatch(url);
    while (it.hasNext())
    {
        auto match = it.next();

        out += url.mid(last, match.capturedStart() - last);

        QString inner = match.captured(1).trimmed();

        // Split by last '|' to support {json:input|jsonPath}
        // We assume that the preceding parameter was `{response}` as other ShareX functions are not supported
        qsizetype pipeIndex = inner.lastIndexOf('|');
        if (pipeIndex != -1)
        {
            inner = inner.mid(pipeIndex + 1).trimmed();
        }

        inner.replace(arrayRegex, R"(.\1)");

        out += '{' + inner + '}';
        last = match.capturedEnd();
        changed = true;
    }

    out += url.mid(last);
    return changed ? out : QString(url);
}

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
        QStringList headerLines = headers.split(';', Qt::SkipEmptyParts);
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
    s.imageUploaderLink = parseUrl(settingsObj["URL"].toString());

    if (settingsObj.contains("DeletionURL") &&
        settingsObj["DeletionURL"].isString())
    {
        s.imageUploaderDeletionLink =
            parseUrl(settingsObj["DeletionURL"].toString());
    }

    if (settingsObj.contains("Headers") && settingsObj["Headers"].isObject())
    {
        QStringList headers = parseHeaders(settingsObj["Headers"].toObject());
        if (!headers.isEmpty())
        {
            s.imageUploaderHeaders = headers.join(';');
        }
    }

    s.imageUploaderEnabled = true;

    return true;
}

ExpectedStr<QJsonObject> validateImportJson(const QString &clipboardText)
{
    if (clipboardText.isEmpty())
    {
        return makeUnexpected("Clipboard must not be empty");
    }

    QJsonParseError parseError;
    QJsonDocument doc =
        QJsonDocument::fromJson(clipboardText.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        return makeUnexpected("Clipboard did not contain valid JSON");
    }

    if (!doc.isObject())
    {
        return makeUnexpected("JSON must be an object");
    }

    auto settingsObj = doc.object();

    if (!settingsObj.contains("Version"))
    {
        return makeUnexpected("JSON must contain the 'Version' key");
    }

    if (!settingsObj.contains("RequestURL"))
    {
        return makeUnexpected("JSON must contain the 'RequestURL' key");
    }

    return settingsObj;
}

}  // namespace chatterino::imageuploader::detail
