#include "singletons/imageuploader/ImageUploader.hpp"

#include "Application.hpp"
#include "common/Env.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "debug/Benchmark.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/imageuploader/UploadedImageModel.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/Result.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"

#include <QBuffer>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QSaveFile>
#include <rapidjson/document.h>

#include <memory>
#include <vector>

#include <utility>

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

using namespace chatterino;
Result<std::vector<UploadedImage>, QString> loadLogFile(
    const QString &logFileName)
{
    //reading existing logs
    QFile logReadFile(logFileName);
    bool isLogFileOkay =
        logReadFile.open(QIODevice::ReadWrite | QIODevice::Text);
    if (!isLogFileOkay)
    {
        return QString("Failed to open log file with links at ") + logFileName;
    }
    auto logs = logReadFile.readAll();
    if (logs.isEmpty())
    {
        logs = QJsonDocument(QJsonArray()).toJson();
    }
    logReadFile.close();
    QJsonArray entries = QJsonDocument::fromJson(logs).array();
    std::vector<UploadedImage> images;
    for (const auto &entry : entries)
    {
        if (!entry.isObject())
        {
            qCWarning(chatterinoImageuploader)
                << "History file contains non-Object JSON data!";
            continue;
        }
        auto obj = entry.toObject();
        images.emplace_back(obj);
    }
    return images;
}

QString getLogFilePath()
{
    return combinePath((getSettings()->logPath.getValue().isEmpty()
                            ? getIApp()->getPaths().messageLogDirectory
                            : getSettings()->logPath),
                       "ImageUploader.json");
}

}  // namespace

namespace chatterino {

// logging information on successful uploads to a json file
void ImageUploader::logToFile(const QString &originalFilePath,
                              const QString &imageLink,
                              const QString &deletionLink, ChannelPtr channel)
{
    const QString logFileName = getLogFilePath();
    auto res = loadLogFile(logFileName);
    if (!res.isOk())
    {
        channel->addMessage(makeSystemMessage(res.error()));
        return;
    }
    auto entries = res.value();
    //writing new data to logs
    UploadedImage img;
    img.channelName = channel->getName();
    img.deletionLink = deletionLink;
    img.imageLink = imageLink;
    img.localPath = originalFilePath;
    img.timestamp = QDateTime::currentSecsSinceEpoch();
    entries.push_back(img);
    // channel name
    // deletion link (can be empty)
    // image link
    // local path to an image (can be empty)
    // timestamp
    QJsonArray arr;
    for (auto &entry : entries)
    {
        arr.append(entry.toJson());
    }
    QSaveFile logSaveFile(logFileName);
    logSaveFile.open(QIODevice::WriteOnly | QIODevice::Text);
    logSaveFile.write(QJsonDocument(arr).toJson());
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

UploadedImageModel *ImageUploader::createModel(QObject *parent)
{
    auto *model = new UploadedImageModel(parent);
    auto res = loadLogFile(getLogFilePath());

    // Replace content of images_
    auto len = this->images_.raw().size();
    for (int i = 0; i < len; i++)
    {
        this->images_.removeAt(0);
    }

    std::vector<UploadedImage> vec = res.valueOr({});
    for (const auto &img : vec)
    {
        this->images_.append(img);
    }

    model->initialize(&this->images_);
    return model;
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
    channel->addMessage(builder.release());
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

        if (source->hasFormat("image/png"))
        {
            // the path to file is not present every time, thus the filePath is empty
            images.push({source->data("image/png"), "png", ""});
            return {images, {}};
        }

        if (source->hasFormat("image/jpeg"))
        {
            images.push({source->data("image/jpeg"), "jpeg", ""});
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
        channel->addMessage(makeSystemMessage(
            QString("Please wait until the upload finishes.")));
        return;
    }

    assert(!images.empty());
    assert(this->uploadQueue_.empty());

    std::swap(this->uploadQueue_, images);

    channel->addMessage(makeSystemMessage("Started upload..."));

    this->sendImageUploadRequest(this->uploadQueue_.front(), std::move(channel),
                                 std::move(outputTextEdit));
    this->uploadQueue_.pop();
}

}  // namespace chatterino
