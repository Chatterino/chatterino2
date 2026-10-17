// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/ImageUploader.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "debug/Benchmark.hpp"
#include "messages/MessageBuilder.hpp"
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

#include <array>
#include <utility>

namespace {

// Delay between uploads in milliseconds
constexpr int UPLOAD_DELAY = 2000;

struct ImageInput {
    QByteArray data;
    QImage image;
    QString format;
    QString filePath;
};

struct SupportedFormat {
    QString mime;
    QString format;
};

const std::array SUPPORTED_FORMATS{
    SupportedFormat{
        .mime = "image/apng",
        .format = "apng",
    },
    SupportedFormat{
        .mime = "image/png",
        .format = "png",
    },
    SupportedFormat{
        .mime = "image/jpeg",
        .format = "jpeg",
    },
    SupportedFormat{
        .mime = "image/gif",
        .format = "gif",
    },
    SupportedFormat{
        .mime = "image/webp",
        .format = "webp",
    },
};

QString uploadFormat(const QMimeType &mime)
{
    for (const auto &format : SUPPORTED_FORMATS)
    {
        if (mime.inherits(format.mime))
        {
            return format.format;
        }
    }
    return {};
}

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

namespace chatterino::imageuploader::detail {

// extracting link to either image or its deletion from response body
QString getJSONValue(QJsonValue responseJson, QStringView jsonPattern)
{
    for (auto key : jsonPattern.tokenize(u'.'))
    {
        if (responseJson.isObject())
        {
            responseJson = responseJson[key];
        }
        else if (responseJson.isArray())
        {
            bool ok = false;
            auto idx = key.toLongLong(&ok);
            if (ok)
            {
                responseJson = responseJson[idx];
            }
        }
        else
        {
            // we reached a scalar value, no need to continue
            break;
        }
    }
    return responseJson.toString();
}

QString getLinkFromResponse(const NetworkResult &response, QString pattern)
{
    QRegularExpression regExp("{(.+)}",
                              QRegularExpression::InvertedGreedinessOption);
    auto match = regExp.match(pattern);

    auto jsonRoot = response.parseJsonValue();
    while (match.hasMatch())
    {
        pattern.replace(match.captured(0),
                        getJSONValue(jsonRoot, match.capturedView(1)));
        match = regExp.match(pattern);
    }
    return pattern;
}

}  // namespace chatterino::imageuploader::detail

namespace chatterino {

using namespace imageuploader::detail;

// logging information on successful uploads to a json file
void ImageUploader::logToFile(const QString &originalFilePath,
                              const QString &imageLink,
                              const QString &deletionLink, ChannelPtr channel)
{
    const QString logFileName =
        combinePath((getSettings()->logPath.getValue().isEmpty()
                         ? getApp()->getPaths().messageLogDirectory
                         : getSettings()->logPath),
                    "ImageUploader.json");

    //reading existing logs
    QFile logReadFile(logFileName);
    bool isLogFileOkay =
        logReadFile.open(QIODevice::ReadWrite | QIODevice::Text);
    if (!isLogFileOkay)
    {
        channel->addSystemMessage(
            QString("Failed to open log file with links at ") + logFileName);
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
    if (!logSaveFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCDebug(chatterinoImageuploader)
            << "Failed to open log file" << logSaveFile.errorString();
        return;
    }
    QJsonArray entries = QJsonDocument::fromJson(logs).array();
    entries.push_back(newLogEntry);
    logSaveFile.write(QJsonDocument(entries).toJson());
    logSaveFile.commit();
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
                   QString(R"(form-data; name="%1"; filename="control_v.%2")")
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

    channel->addSystemMessage(errorMessage);
    // NOTE: We abort any future uploads on failure. Should this be handled differently?
    while (!this->uploadQueue_.empty())
    {
        this->uploadQueue_.pop();
    }
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
    channel->addMessage(builder.release(), MessageContext::Original);
    if (this->uploadQueue_.empty())
    {
        this->uploadMutex_.unlock();
    }
    else
    {
        QTimer::singleShot(UPLOAD_DELAY, [channel, textEdit, this]() {
            this->sendImageUploadRequest(this->uploadQueue_.front(), channel,
                                         textEdit);
            this->uploadQueue_.pop();
        });
    }

    this->logToFile(originalFilePath, link, deletionLink, channel);
}

std::pair<std::queue<RawImageData>, QString> ImageUploader::getImages(
    const QMimeData *source) const
{
    BenchmarkGuard benchmarkGuard("ImageUploader::getImages");

    auto getImagesFromUrls =
        [&]() -> std::pair<std::queue<ImageInput>, QString> {
        if (!source->hasUrls())
        {
            return {{}, {}};
        }

        std::queue<ImageInput> images;

        auto mimeDb = QMimeDatabase();
        // This path gets chosen when files are copied from a file manager, like explorer.exe, caja.
        // Each entry in source->urls() is a QUrl pointing to a file that was copied.
        for (const QUrl &path : source->urls())
        {
            QString localPath = path.toLocalFile();
            QMimeType mime = mimeDb.mimeTypeForUrl(path);
            const auto format = uploadFormat(mime);
            if (!mime.name().startsWith("image") && format.isEmpty())
            {
                continue;
            }

            if (format.isEmpty())
            {
                images.push({
                    .image = QImage(localPath),
                    .filePath = localPath,
                });
                continue;
            }

            QFile file(localPath);
            if (!file.open(QIODevice::ReadOnly))
            {
                return {{}, "Failed to open file :("};
            }
            images.push({
                .data = file.readAll(),
                .format = format,
                .filePath = localPath,
            });
        }

        return {images, {}};
    };

    auto getImagesDirectly =
        [&]() -> std::pair<std::queue<ImageInput>, QString> {
        std::queue<ImageInput> images;

        for (const auto &format : SUPPORTED_FORMATS)
        {
            if (source->hasFormat(format.mime))
            {
                images.push({
                    .data = source->data(format.mime),
                    .format = format.format,
                });
                return {images, {}};
            }
        }

        if (source->hasImage())
        {
            images.push({
                .image = qvariant_cast<QImage>(source->imageData()),
            });
        }
        return {images, images.empty()
                            ? "Cannot upload file, failed to convert to png."
                            : QString{}};
    };

    auto [inputImages, urlError] = getImagesFromUrls();
    QString directError;
    if (inputImages.empty())
    {
        auto directImages = getImagesDirectly();
        inputImages = std::move(directImages.first);
        directError = std::move(directImages.second);
    }

    if (inputImages.empty())
    {
        return {{}, urlError + directError};
    }

    std::queue<RawImageData> images;
    while (!inputImages.empty())
    {
        auto input = std::move(inputImages.front());
        inputImages.pop();

        if (!input.format.isEmpty())
        {
            images.push({std::move(input.data), std::move(input.format),
                         std::move(input.filePath)});
            continue;
        }

        if (input.image.isNull())
        {
            input.image = QImage::fromData(input.data);
        }
        if (input.image.isNull())
        {
            return {{},
                    input.filePath.isEmpty()
                        ? "Cannot upload file, failed to convert to png."
                        : "Couldn't load image :("};
        }

        auto imageData = convertToPng(input.image);
        if (!imageData)
        {
            if (!input.filePath.isEmpty())
            {
                return {
                    {},
                    QString("Cannot upload file: %1. Couldn't convert image "
                            "to png.")
                        .arg(input.filePath),
                };
            }
            return {{}, "Cannot upload file, failed to convert to png."};
        }
        images.push({*imageData, "png", std::move(input.filePath)});
    }

    return {images, {}};
}

void ImageUploader::upload(std::queue<RawImageData> images, ChannelPtr channel,
                           QPointer<ResizingTextEdit> outputTextEdit)
{
    BenchmarkGuard benchmarkGuard("upload");
    if (!this->uploadMutex_.tryLock())
    {
        channel->addSystemMessage("Please wait until the upload finishes.");
        return;
    }

    assert(!images.empty());
    assert(this->uploadQueue_.empty());

    std::swap(this->uploadQueue_, images);

    channel->addSystemMessage("Started upload...");

    this->sendImageUploadRequest(this->uploadQueue_.front(), std::move(channel),
                                 std::move(outputTextEdit));
    this->uploadQueue_.pop();
}

}  // namespace chatterino
