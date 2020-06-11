#include "NuulsUploader.hpp"

#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QBuffer>
#include <QHttpMultiPart>
#include <QMimeDatabase>
#include <QMutex>

#define UPLOAD_DELAY 2000
// Delay between uploads in milliseconds

namespace {

boost::optional<QByteArray> convertToPng(QImage image)
{
    QByteArray imageData;
    QBuffer buf(&imageData);
    buf.open(QIODevice::WriteOnly);
    bool success = image.save(&buf, "png");
    if (success)
    {
        return boost::optional<QByteArray>(imageData);
    }
    else
    {
        return boost::optional<QByteArray>(boost::none);
    }
}
}  // namespace

namespace chatterino {
// These variables are only used from the main thread.
static auto uploadMutex = QMutex();
static std::queue<RawImageData> uploadQueue;

//logging information on successful uploads to a csv file
void logToCsv(const QString originalFilePath, const QString link,
              ChannelPtr channel)
{
    const QString csvFileName = (getSettings()->logPath.getValue().isEmpty()
                                     ? getPaths()->messageLogDirectory
                                     : getSettings()->logPath) +
                                "/ImageUploader.csv";
    QFile csvFile(csvFileName);
    bool csvExisted = csvFile.exists();
    bool isCsvOkay = csvFile.open(QIODevice::Append | QIODevice::Text);
    if (!isCsvOkay)
    {
        channel->addMessage(makeSystemMessage(
            QString("Failed to open csv file with links at ") + csvFileName));
        return;
    }
    QTextStream out(&csvFile);
    qDebug() << csvExisted;
    if (!csvExisted)
    {
        out << "localPath,imageLink,timestamp,channelName\n";
    }
    out << originalFilePath + QString(",") << link + QString(",")
        << QDateTime::currentSecsSinceEpoch()
        << QString(",%1\n").arg(channel->getName());
    // image path (can be empty)
    // image link
    // timestamp
    // channel name
    csvFile.close();
}

void uploadImageToNuuls(RawImageData imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit)
{
    const static char *const boundary = "thisistheboudaryasd";
    const static QString contentType =
        QString("multipart/form-data; boundary=%1").arg(boundary);
    static QUrl url(Env::get().imageUploaderUrl);
    static QString formBody(Env::get().imageUploaderFormBody);
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
                       .arg(formBody)
                       .arg(imageData.format));
    payload->setBoundary(boundary);
    payload->append(part);
    NetworkRequest(url, NetworkRequestType::Post)
        .header("Content-Type", contentType)

        .multiPart(payload)
        .onSuccess([&textEdit, channel,
                    originalFilePath](NetworkResult result) -> Outcome {
            textEdit.insertPlainText(result.getData() + QString(" "));
            if (uploadQueue.empty())
            {
                channel->addMessage(makeSystemMessage(
                    QString("Your image has been uploaded to ") +
                    result.getData()));
                uploadMutex.unlock();
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    QString(
                        "Your image has been uploaded to %1 . %2 left. Please "
                        "wait until all of them are uploaded. About %3 "
                        "seconds left.")
                        .arg(result.getData() + QString(""))
                        .arg(uploadQueue.size())
                        .arg(uploadQueue.size() * (UPLOAD_DELAY / 1000 + 1))));
                // 2 seconds for the timer that's there not to spam the remote server
                // and 1 second of actual uploading.

                QTimer::singleShot(UPLOAD_DELAY, [channel, &textEdit]() {
                    uploadImageToNuuls(uploadQueue.front(), channel, textEdit);
                    uploadQueue.pop();
                });
            }

            logToCsv(originalFilePath, result.getData(), channel);

            return Success;
        })
        .onError([channel](NetworkResult result) -> bool {
            channel->addMessage(makeSystemMessage(
                QString("An error happened while uploading your image: %1")
                    .arg(result.status())));
            uploadMutex.unlock();
            return true;
        })
        .execute();
}

void upload(const QMimeData *source, ChannelPtr channel,
            ResizingTextEdit &outputTextEdit)
{
    if (!uploadMutex.tryLock())
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
            qDebug() << mime.name();
            if (mime.name().startsWith("image") && !mime.inherits("image/gif"))
            {
                channel->addMessage(makeSystemMessage(
                    QString("Uploading image: %1").arg(localPath)));
                QImage img = QImage(localPath);
                if (img.isNull())
                {
                    channel->addMessage(
                        makeSystemMessage(QString("Couldn't load image :(")));
                    uploadMutex.unlock();
                    return;
                }

                boost::optional<QByteArray> imageData = convertToPng(img);
                if (imageData)
                {
                    RawImageData data = {imageData.get(), "png", localPath};
                    uploadQueue.push(data);
                }
                else
                {
                    channel->addMessage(makeSystemMessage(
                        QString("Cannot upload file: %1. Couldn't convert "
                                "image to png.")
                            .arg(localPath)));
                    uploadMutex.unlock();
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
                    uploadMutex.unlock();
                    return;
                }
                RawImageData data = {file.readAll(), "gif", localPath};
                uploadQueue.push(data);
                file.close();
                // file.readAll() => might be a bit big but it /should/ work
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    QString("Cannot upload file: %1. Not an image.")
                        .arg(localPath)));
                uploadMutex.unlock();
                return;
            }
        }
        if (!uploadQueue.empty())
        {
            uploadImageToNuuls(uploadQueue.front(), channel, outputTextEdit);
            uploadQueue.pop();
        }
    }
    else if (source->hasFormat("image/png"))
    {
        // the path to file is not present every time, thus the filePath is empty
        uploadImageToNuuls({source->data("image/png"), "png", ""}, channel,
                           outputTextEdit);
    }
    else if (source->hasFormat("image/jpeg"))
    {
        uploadImageToNuuls({source->data("image/jpeg"), "jpeg", ""}, channel,
                           outputTextEdit);
    }
    else if (source->hasFormat("image/gif"))
    {
        uploadImageToNuuls({source->data("image/gif"), "gif", ""}, channel,
                           outputTextEdit);
    }

    else
    {  // not PNG, try loading it into QImage and save it to a PNG.
        QImage image = qvariant_cast<QImage>(source->imageData());
        boost::optional<QByteArray> imageData = convertToPng(image);
        if (imageData)
        {
            uploadImageToNuuls({imageData.get(), "png", ""}, channel,
                               outputTextEdit);
        }
        else
        {
            channel->addMessage(makeSystemMessage(
                QString("Cannot upload file, failed to convert to png.")));
            uploadMutex.unlock();
        }
    }
}

}  // namespace chatterino
