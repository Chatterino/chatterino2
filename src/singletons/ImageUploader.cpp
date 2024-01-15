#include "singletons/ImageUploader.hpp"

#include "common/Env.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"

#include <QBuffer>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QMutex>
#include <QPointer>
#include <QSaveFile>

#define UPLOAD_DELAY 2000
// Delay between uploads in milliseconds

namespace {

std::optional<QByteArray> convertToPng(const QImage &image)
{
    QByteArray imageData;
    QBuffer buf(&imageData);
    buf.open(QIODevice::WriteOnly);
    bool success = image.save(&buf, "png");
    if (success)
    {
        return imageData;
    }

    return std::nullopt;
}

}  // namespace

namespace chatterino {

// logging information on successful uploads to a json file
void ImageUploader::logToFile(const QString &originalFilePath,
                              const QString &imageLink,
                              const QString &deletionLink, ChannelPtr channel)
{
    const QString logFileName =
        combinePath((getSettings()->logPath.getValue().isEmpty()
                         ? getPaths()->messageLogDirectory
                         : getSettings()->logPath),
                    "ImageUploader.json");

    //reading existing logs
    QFile logReadFile(logFileName);
    bool isLogFileOkay =
        logReadFile.open(QIODevice::ReadWrite | QIODevice::Text);
    if (!isLogFileOkay)
    {
        channel->addMessage(makeSystemMessage(
            QString("Failed to open log file with links at ") + logFileName));
        return;
    }
    auto logs = logReadFile.readAll();
    if (logs.isEmpty())
    {
        logs = QJsonDocument(QJsonArray()).toJson();
    }
    logReadFile.close();

    //writing new data to logs
    QJsonObject newLogEntry;
    newLogEntry["channelName"] = channel->getName();
    newLogEntry["deletionLink"] =
        deletionLink.isEmpty() ? QJsonValue(QJsonValue::Null) : deletionLink;
    newLogEntry["imageLink"] = imageLink;
    newLogEntry["localPath"] = originalFilePath.isEmpty()
                                   ? QJsonValue(QJsonValue::Null)
                                   : originalFilePath;
    newLogEntry["timestamp"] = QDateTime::currentSecsSinceEpoch();
    // channel name
    // deletion link (can be empty)
    // image link
    // local path to an image (can be empty)
    // timestamp
    QSaveFile logSaveFile(logFileName);
    logSaveFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QJsonArray entries = QJsonDocument::fromJson(logs).array();
    entries.push_back(newLogEntry);
    logSaveFile.write(QJsonDocument(entries).toJson());
    logSaveFile.commit();
}

// extracting link to either image or its deletion from response body
QString getJSONValue(QJsonValue responseJson, QString jsonPattern)
{
    for (const QString &key : jsonPattern.split("."))
    {
        responseJson = responseJson[key];
    }
    return responseJson.toString();
}

QString getLinkFromResponse(NetworkResult response, QString pattern)
{
    QRegularExpression regExp("{(.+)}",
                              QRegularExpression::InvertedGreedinessOption);
    auto match = regExp.match(pattern);

    while (match.hasMatch())
    {
        pattern.replace(match.captured(0),
                        getJSONValue(response.parseJson(), match.captured(1)));
        match = regExp.match(pattern);
    }
    return pattern;
}

void ImageUploader::save()
{
}

void ImageUploader::sendImageUploadRequest(RawImageData imageData,
                                           ChannelPtr channel,
                                           QPointer<ResizingTextEdit> textEdit)
{
    QUrl url(getSettings()->imageUploaderUrl.getValue().isEmpty()
                 ? getSettings()->imageUploaderUrl.getDefaultValue()
                 : getSettings()->imageUploaderUrl);
    QString formField(
        getSettings()->imageUploaderFormField.getValue().isEmpty()
            ? getSettings()->imageUploaderFormField.getDefaultValue()
            : getSettings()->imageUploaderFormField);
    auto extraHeaders =
        parseHeaderList(getSettings()->imageUploaderHeaders.getValue());
    QString originalFilePath = imageData.filePath;

    QHttpMultiPart *payload = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part = QHttpPart();
    part.setBody(imageData.data);
    part.setHeader(QNetworkRequest::ContentTypeHeader,
                   QString("image/%1").arg(imageData.format));
    part.setHeader(QNetworkRequest::ContentLengthHeader,
                   QVariant(imageData.data.length()));
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QString("form-data; name=\"%1\"; filename=\"control_v.%2\"")
                       .arg(formField)
                       .arg(imageData.format));
    payload->append(part);

    NetworkRequest(url, NetworkRequestType::Post)
        .headerList(extraHeaders)
        .multiPart(payload)
        .onSuccess(
            [textEdit, channel, originalFilePath, this](NetworkResult result) {
                this->handleSuccessfulUpload(result, originalFilePath, channel,
                                             textEdit);
            })
        .onError([channel, this](NetworkResult result) -> bool {
            this->handleFailedUpload(result, channel);
            return true;
        })
        .execute();
}

void ImageUploader::handleFailedUpload(const NetworkResult &result,
                                       ChannelPtr channel)
{
    auto errorMessage =
        QString("An error happened while uploading your image: %1")
            .arg(result.formatError());

    // Try to read more information from the result body
    auto obj = result.parseJson();
    if (!obj.isEmpty())
    {
        auto apiCode = obj.value("code");
        if (!apiCode.isUndefined())
        {
            auto codeString = apiCode.toVariant().toString();
            codeString.truncate(20);
            errorMessage += QString(" - code: %1").arg(codeString);
        }

        auto apiError = obj.value("error").toString();
        if (!apiError.isEmpty())
        {
            apiError.truncate(300);
            errorMessage += QString(" - error: %1").arg(apiError.trimmed());
        }
    }

    channel->addMessage(makeSystemMessage(errorMessage));
    this->uploadMutex_.unlock();
}

void ImageUploader::handleSuccessfulUpload(const NetworkResult &result,
                                           QString originalFilePath,
                                           ChannelPtr channel,
                                           QPointer<ResizingTextEdit> textEdit)
{
    if (textEdit == nullptr)
    {
        // Split was destroyed abort further uploads

        while (!this->uploadQueue_.empty())
        {
            this->uploadQueue_.pop();
        }
        this->uploadMutex_.unlock();
        return;
    }
    QString link =
        getSettings()->imageUploaderLink.getValue().isEmpty()
            ? result.getData()
            : getLinkFromResponse(result, getSettings()->imageUploaderLink);
    QString deletionLink =
        getSettings()->imageUploaderDeletionLink.getValue().isEmpty()
            ? ""
            : getLinkFromResponse(result,
                                  getSettings()->imageUploaderDeletionLink);
    qCDebug(chatterinoImageuploader) << link << deletionLink;
    textEdit->insertPlainText(link + " ");

    // 2 seconds for the timer that's there not to spam the remote server
    // and 1 second of actual uploading.
    auto timeToUpload = this->uploadQueue_.size() * (UPLOAD_DELAY / 1000 + 1);
    MessageBuilder builder(imageUploaderResultMessage, link, deletionLink,
                           this->uploadQueue_.size(), timeToUpload);
    channel->addMessage(builder.release());
    if (this->uploadQueue_.empty())
    {
        this->uploadMutex_.unlock();
    }
    else
    {
        QTimer::singleShot(UPLOAD_DELAY, [channel, &textEdit, this]() {
            this->sendImageUploadRequest(this->uploadQueue_.front(), channel,
                                         textEdit);
            this->uploadQueue_.pop();
        });
    }

    this->logToFile(originalFilePath, link, deletionLink, channel);
}

void ImageUploader::upload(const QMimeData *source, ChannelPtr channel,
                           QPointer<ResizingTextEdit> outputTextEdit)
{
    if (!this->uploadMutex_.tryLock())
    {
        channel->addMessage(makeSystemMessage(
            QString("Please wait until the upload finishes.")));
        return;
    }

    channel->addMessage(makeSystemMessage(QString("Started upload...")));
    if (source->hasUrls())
    {
        auto mimeDb = QMimeDatabase();
        // This path gets chosen when files are copied from a file manager, like explorer.exe, caja.
        // Each entry in source->urls() is a QUrl pointing to a file that was copied.
        for (const QUrl &path : source->urls())
        {
            QString localPath = path.toLocalFile();
            QMimeType mime = mimeDb.mimeTypeForUrl(path);
            if (mime.name().startsWith("image") && !mime.inherits("image/gif"))
            {
                channel->addMessage(makeSystemMessage(
                    QString("Uploading image: %1").arg(localPath)));
                QImage img = QImage(localPath);
                if (img.isNull())
                {
                    channel->addMessage(
                        makeSystemMessage(QString("Couldn't load image :(")));
                    this->uploadMutex_.unlock();
                    return;
                }

                auto imageData = convertToPng(img);
                if (imageData)
                {
                    RawImageData data = {*imageData, "png", localPath};
                    this->uploadQueue_.push(data);
                }
                else
                {
                    channel->addMessage(makeSystemMessage(
                        QString("Cannot upload file: %1. Couldn't convert "
                                "image to png.")
                            .arg(localPath)));
                    this->uploadMutex_.unlock();
                    return;
                }
            }
            else if (mime.inherits("image/gif"))
            {
                channel->addMessage(makeSystemMessage(
                    QString("Uploading GIF: %1").arg(localPath)));
                QFile file(localPath);
                bool isOkay = file.open(QIODevice::ReadOnly);
                if (!isOkay)
                {
                    channel->addMessage(
                        makeSystemMessage(QString("Failed to open file. :(")));
                    this->uploadMutex_.unlock();
                    return;
                }
                RawImageData data = {file.readAll(), "gif", localPath};
                this->uploadQueue_.push(data);
                file.close();
                // file.readAll() => might be a bit big but it /should/ work
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    QString("Cannot upload file: %1. Not an image.")
                        .arg(localPath)));
                this->uploadMutex_.unlock();
                return;
            }
        }
        if (!this->uploadQueue_.empty())
        {
            this->sendImageUploadRequest(this->uploadQueue_.front(), channel,
                                         outputTextEdit);
            this->uploadQueue_.pop();
        }
    }
    else if (source->hasFormat("image/png"))
    {
        // the path to file is not present every time, thus the filePath is empty
        this->sendImageUploadRequest({source->data("image/png"), "png", ""},
                                     channel, outputTextEdit);
    }
    else if (source->hasFormat("image/jpeg"))
    {
        this->sendImageUploadRequest({source->data("image/jpeg"), "jpeg", ""},
                                     channel, outputTextEdit);
    }
    else if (source->hasFormat("image/gif"))
    {
        this->sendImageUploadRequest({source->data("image/gif"), "gif", ""},
                                     channel, outputTextEdit);
    }

    else
    {  // not PNG, try loading it into QImage and save it to a PNG.
        auto image = qvariant_cast<QImage>(source->imageData());
        auto imageData = convertToPng(image);
        if (imageData)
        {
            sendImageUploadRequest({*imageData, "png", ""}, channel,
                                   outputTextEdit);
        }
        else
        {
            channel->addMessage(makeSystemMessage(
                QString("Cannot upload file, failed to convert to png.")));
            this->uploadMutex_.unlock();
        }
    }
}

}  // namespace chatterino
