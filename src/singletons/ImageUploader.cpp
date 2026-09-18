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
#include <QtEndian>

#include <utility>

namespace {

// Delay between uploads in milliseconds
constexpr int UPLOAD_DELAY = 2000;

bool isAnimatedPng(QFile &file)
{
    // An APNG has an 8 byte acTL chunk before the first IDAT chunk
    // https://www.w3.org/TR/png-3/#structure
    // https://www.w3.org/TR/png-3/#acTL-chunk
    constexpr QByteArrayView signature{"\x89PNG\r\n\x1a\n", 8};
    if (file.read(signature.size()) != signature)
    {
        return false;
    }

    // Read each chunk's length and type, skip its data
    while (file.size() - file.pos() >= 12)
    {
        const auto header = file.read(8);
        if (header.size() != 8)
        {
            return false;
        }
        const auto length = qFromBigEndian<quint32>(header.constData());
        if (length > file.size() - file.pos() - 4)
        {
            return false;
        }
        const auto type = QByteArrayView{header}.sliced(4, 4);
        if (type == "acTL")
        {
            return length == 8;
        }
        if (type == "IDAT" || type == "IEND")
        {
            return false;
        }
        if (!file.seek(file.pos() + length + 4))
        {
            return false;
        }
    }
    return false;
}

bool isAnimatedWebp(QByteArrayView data)
{
    // Extended WebPs have a VP8X chunk after a 12 byte WebP header
    // The animation flag is bit 1 of the first byte in the VP8X data
    // https://developers.google.com/speed/webp/docs/riff_container#extended_file_format
    constexpr qsizetype flagsOffset = 20;
    constexpr quint32 vp8xDataSize = 10;
    constexpr uchar animationFlag = 0x02;

    if (data.size() <= flagsOffset)
    {
        return false;
    }
    if (data.first(4) != "RIFF" || data.sliced(8, 4) != "WEBP")
    {
        return false;
    }
    if (data.sliced(12, 4) != "VP8X")
    {
        return false;
    }
    if (qFromLittleEndian<quint32>(data.data() + 16) != vp8xDataSize)
    {
        return false;
    }
    return (static_cast<uchar>(data[flagsOffset]) & animationFlag) != 0;
}

std::optional<QByteArray> stripPngExif(QByteArrayView data)
{
    // PNG stores EXIF in an eXIf chunk. The other chunks can be copied without
    // recalculating their CRCs.
    // https://www.w3.org/TR/png-3/#eXIf-chunk
    constexpr QByteArrayView signature{"\x89PNG\r\n\x1a\n", 8};
    if (!data.startsWith(signature))
    {
        return std::nullopt;
    }

    QByteArray stripped;
    stripped.reserve(data.size());
    stripped.append(signature);

    // Copy every complete chunk except eXIf.
    auto offset = signature.size();
    while (offset < data.size())
    {
        const auto remaining = data.size() - offset;
        if (remaining < 12)
        {
            return std::nullopt;
        }

        const auto length = qFromBigEndian<quint32>(data.data() + offset);
        if (length > remaining - 12)
        {
            return std::nullopt;
        }

        const auto chunkSize = static_cast<qsizetype>(length) + 12;
        const auto type = data.sliced(offset + 4, 4);
        if (type != "eXIf")
        {
            stripped.append(data.sliced(offset, chunkSize));
        }
        offset += chunkSize;
    }

    return stripped;
}

std::optional<QByteArray> stripWebpExif(QByteArrayView data)
{
    // WebP stores EXIF in a RIFF chunk. Removing it also requires updating the
    // RIFF size and the EXIF flag in VP8X.
    // https://developers.google.com/speed/webp/docs/riff_container#metadata
    constexpr qsizetype headerSize = 12;
    constexpr qsizetype flagsOffset = 20;
    constexpr uchar exifFlag = 0x08;

    if (data.size() < headerSize || data.first(4) != "RIFF" ||
        data.sliced(8, 4) != "WEBP" ||
        qFromLittleEndian<quint32>(data.data() + 4) != data.size() - 8)
    {
        return std::nullopt;
    }

    QByteArray stripped;
    stripped.reserve(data.size());
    stripped.append(data.first(headerSize));

    // RIFF chunks are padded to an even size.
    auto offset = headerSize;
    while (offset < data.size())
    {
        const auto remaining = data.size() - offset;
        if (remaining < 8)
        {
            return std::nullopt;
        }

        const auto length =
            qFromLittleEndian<quint32>(data.data() + offset + 4);
        const auto chunkSize =
            static_cast<qsizetype>(length) + 8 + (length & 1U);
        if (chunkSize > remaining)
        {
            return std::nullopt;
        }

        if (data.sliced(offset, 4) != "EXIF")
        {
            stripped.append(data.sliced(offset, chunkSize));
        }
        offset += chunkSize;
    }

    // Clear the EXIF flag and update the size after removing the chunk.
    if (stripped.size() <= flagsOffset ||
        QByteArrayView{stripped}.sliced(12, 4) != "VP8X")
    {
        return std::nullopt;
    }

    stripped[flagsOffset] = static_cast<char>(
        static_cast<uchar>(stripped[flagsOffset]) & ~exifFlag);
    qToLittleEndian(static_cast<quint32>(stripped.size() - 8),
                    stripped.data() + 4);
    return stripped;
}

std::optional<QByteArray> stripJpegExif(QByteArrayView data)
{
    // JPEG stores EXIF in APP1 segments before the image data.
    // We only strip the APP1 segments with EXIF data.
    constexpr QByteArrayView startOfImage{"\xFF\xD8", 2};
    constexpr QByteArrayView exifHeader{"Exif\0\0", 6};
    if (!data.startsWith(startOfImage))
    {
        return std::nullopt;
    }

    QByteArray stripped;
    stripped.reserve(data.size());
    stripped.append(startOfImage);

    // Copy each marker segment except EXIF APP1 segments.
    auto offset = startOfImage.size();
    while (offset < data.size())
    {
        const auto markerStart = offset;
        while (offset < data.size() && static_cast<uchar>(data[offset]) == 0xFF)
        {
            ++offset;
        }
        if (offset >= data.size())
        {
            return std::nullopt;
        }

        const auto marker = static_cast<uchar>(data[offset++]);
        if (marker == 0xDA)
        {
            // EXIF cannot appear in the compressed image data.
            stripped.append(data.sliced(markerStart));
            return stripped;
        }
        if (marker == 0xD9)
        {
            stripped.append(data.sliced(markerStart, offset - markerStart));
            return offset == data.size() ? std::optional{stripped}
                                         : std::nullopt;
        }
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8))
        {
            stripped.append(data.sliced(markerStart, offset - markerStart));
            continue;
        }
        if (data.size() - offset < 2)
        {
            return std::nullopt;
        }

        const auto length = qFromBigEndian<quint16>(data.data() + offset);
        if (length < 2 || length > data.size() - offset)
        {
            return std::nullopt;
        }

        const auto payload = data.sliced(offset + 2, length - 2);
        if (marker != 0xE1 || !payload.startsWith(exifHeader))
        {
            stripped.append(
                data.sliced(markerStart, offset + length - markerStart));
        }
        offset += length;
    }

    return std::nullopt;
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

    auto tryUploadFromUrls =
        [&]() -> std::pair<std::queue<RawImageData>, QString> {
        if (!source->hasUrls())
        {
            return {{}, {}};
        }

        std::queue<RawImageData> images;

        auto mimeDb = QMimeDatabase();
        // This path gets chosen when files are copied from a file manager, like explorer.exe, caja.
        // Each entry in source->urls() is a QUrl pointing to a file that was copied.
        for (const QUrl &path : source->urls())
        {
            QString localPath = path.toLocalFile();
            QMimeType mime = mimeDb.mimeTypeForUrl(path);
            // .apng files are not always reported as image/png.
            if (mime.inherits("image/png") ||
                localPath.endsWith(".apng", Qt::CaseInsensitive))
            {
                QFile file(localPath);
                if (!file.open(QIODevice::ReadOnly))
                {
                    return {{}, "Failed to open file :("};
                }
                if (isAnimatedPng(file))
                {
                    file.seek(0);
                    auto data = stripPngExif(file.readAll());
                    if (!data)
                    {
                        return {{}, "Failed to read image :("};
                    }
                    images.push({std::move(*data), "apng", localPath});
                    continue;
                }
            }
            if (mime.inherits("image/webp"))
            {
                QFile file(localPath);
                if (!file.open(QIODevice::ReadOnly))
                {
                    return {{}, "Failed to open file :("};
                }
                if (isAnimatedWebp(file.read(21)))
                {
                    file.seek(0);
                    auto data = stripWebpExif(file.readAll());
                    if (!data)
                    {
                        return {{}, "Failed to read image :("};
                    }
                    images.push({std::move(*data), "webp", localPath});
                    continue;
                }
            }
            if (mime.name().startsWith("image") && !mime.inherits("image/gif"))
            {
                QImage img = QImage(localPath);
                if (img.isNull())
                {
                    return {{}, "Couldn't load image :("};
                }

                auto imageData = convertToPng(img);
                if (!imageData)
                {
                    return {
                        {},
                        QString("Cannot upload file: %1. Couldn't convert "
                                "image to png.")
                            .arg(localPath),
                    };
                }
                images.push({*imageData, "png", localPath});
            }
            else if (mime.inherits("image/gif"))
            {
                QFile file(localPath);
                bool isOkay = file.open(QIODevice::ReadOnly);
                if (!isOkay)
                {
                    return {{}, "Failed to open file :("};
                }
                // file.readAll() => might be a bit big but it /should/ work
                images.push({file.readAll(), "gif", localPath});
                file.close();
            }
        }

        return {images, {}};
    };

    auto tryUploadDirectly =
        [&]() -> std::pair<std::queue<RawImageData>, QString> {
        std::queue<RawImageData> images;

        if (source->hasFormat("image/apng"))
        {
            auto data = stripPngExif(source->data("image/apng"));
            if (!data)
            {
                return {{}, "Failed to read image :("};
            }
            images.push({std::move(*data), "apng", ""});
            return {images, {}};
        }

        if (source->hasFormat("image/webp"))
        {
            auto data = source->data("image/webp");
            if (isAnimatedWebp(data))
            {
                auto stripped = stripWebpExif(data);
                if (!stripped)
                {
                    return {{}, "Failed to read image :("};
                }
                images.push({std::move(*stripped), "webp", ""});
                return {images, {}};
            }
        }

        if (source->hasFormat("image/png"))
        {
            // the path to file is not present every time, thus the filePath is empty
            auto data = stripPngExif(source->data("image/png"));
            if (!data)
            {
                return {{}, "Failed to read image :("};
            }
            images.push({std::move(*data), "png", ""});
            return {images, {}};
        }

        if (source->hasFormat("image/jpeg"))
        {
            auto data = stripJpegExif(source->data("image/jpeg"));
            if (!data)
            {
                return {{}, "Failed to read image :("};
            }
            images.push({std::move(*data), "jpeg", ""});
            return {images, {}};
        }

        if (source->hasFormat("image/gif"))
        {
            images.push({source->data("image/gif"), "gif", ""});
            return {images, {}};
        }

        // not PNG, try loading it into QImage and save it to a PNG.
        auto image = qvariant_cast<QImage>(source->imageData());
        auto imageData = convertToPng(image);
        if (imageData)
        {
            images.push({*imageData, "png", ""});
            return {images, {}};
        }

        // No direct upload happenned
        return {{}, "Cannot upload file, failed to convert to png."};
    };

    const auto [urlImageData, urlError] = tryUploadFromUrls();

    if (!urlImageData.empty())
    {
        return {urlImageData, {}};
    }

    const auto [directImageData, directError] = tryUploadDirectly();
    if (!directImageData.empty())
    {
        return {directImageData, {}};
    }

    return {
        {},
        // TODO: verify that this looks ok xd
        urlError + directError,
    };
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
