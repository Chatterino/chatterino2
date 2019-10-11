#include "NuulsUploader.hpp"

#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"

#include <QHttpMultiPart>

#define UPLOAD_DELAY 2000
// Delay between uploads in milliseconds

namespace {

QString getImageFileFormat(QString path)
{
    static QStringList listOfImageFormats = {".png", ".jpg", ".jpeg"};
    for (const QString &format : listOfImageFormats)
    {
        if (path.endsWith(format))
        {
            return format.mid(1);
        }
    }
    return QString();
}

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
bool isUploading = false;

std::queue<TypedBytes> uploadQueue;

void uploadImageToNuuls(TypedBytes imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit)
{
    const char *boundary = "thisistheboudaryasd";
    QString contentType =
        QString("multipart/form-data; boundary=%1").arg(boundary);
    static QUrl url(Env::get().imageUploaderUrl);

    QHttpMultiPart *payload = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part = QHttpPart();
    part.setBody(imageData.data);
    part.setHeader(QNetworkRequest::ContentTypeHeader,
                   QString("image/%1").arg(imageData.type));
    part.setHeader(QNetworkRequest::ContentLengthHeader,
                   QVariant(imageData.data.length()));
    part.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QString("form-data; name=\"attachment\"; filename=\"control_v.%1\"")
            .arg(imageData.type));
    payload->setBoundary(boundary);
    payload->append(part);
    NetworkRequest(url, NetworkRequestType::Post)
        .header("Content-Type", contentType)

        .multiPart(payload)
        .onSuccess([&textEdit, channel](NetworkResult result) -> Outcome {
            textEdit.insertPlainText(result.getData() + QString(" "));
            isUploading = false;
            if (uploadQueue.empty())
            {
                channel->addMessage(makeSystemMessage(
                    QString("Your image has been uploaded.")));
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    QString("Your image has been uploaded. %1 left. Please "
                            "wait until all of them are uploaded. About %2 "
                            "seconds left.")
                        .arg(uploadQueue.size(),
                             uploadQueue.size() *
                                 (UPLOAD_DELAY / 1000 +
                                  1)  // convert UPLOAD_DELAY to seconds
                             )));
                // Argument number 2 is the ETA.
                // 2 seconds for the timer that's there not to spam Nuuls' server
                // and 1 second of actual uploading.

                QTimer::singleShot(UPLOAD_DELAY, [channel, &textEdit]() {
                    uploadImageToNuuls(uploadQueue.front(), channel, textEdit);
                    uploadQueue.pop();
                });
            }
            return Success;
        })
        .onError([channel](NetworkResult result) -> bool {
            channel->addMessage(makeSystemMessage(
                QString("An error happened while uploading your image: %1")
                    .arg(result.status())));
            isUploading = false;
            return true;
        })
        .execute();
}

void upload(const QMimeData *source, ChannelPtr channel,
            ResizingTextEdit &outputTextEdit)
{
    if (isUploading)
    {
        channel->addMessage(makeSystemMessage(
            QString("Please wait until the upload finishes.")));
        return;
    }

    isUploading = true;
    channel->addMessage(makeSystemMessage(QString("Started upload...")));

    if (source->hasFormat("image/png"))
    {
        uploadImageToNuuls({source->data("image/png"), "png"}, channel,
                           outputTextEdit);
    }
    else if (source->hasFormat("image/jpeg"))
    {
        uploadImageToNuuls({source->data("image/jpeg"), "jpeg"}, channel,
                           outputTextEdit);
    }
    else if (source->hasFormat("image/gif"))
    {
        uploadImageToNuuls({source->data("image/gif"), "gif"}, channel,
                           outputTextEdit);
    }
    else if (source->hasUrls())
    {
        // This path gets chosen when files are copied from a file manager, like explorer.exe, caja.
        // Each entry in source->urls() is a QUrl pointng to a file that was copied.
        for (const QUrl &path : source->urls())
        {
            if (!getImageFileFormat(path.toLocalFile()).isEmpty())
            {
                channel->addMessage(makeSystemMessage(
                    QString("Uploading image: %1").arg(path.toLocalFile())));
                QImage img = QImage(path.toLocalFile());
                if (img.isNull())
                {
                    channel->addMessage(
                        makeSystemMessage(QString("Couldn't load image :(")));
                    return;
                }

                boost::optional<QByteArray> imageData = convertToPng(img);
                if (imageData)
                {
                    TypedBytes data = {imageData.get(), "png"};
                    uploadQueue.push(data);
                }
                else
                {
                    channel->addMessage(makeSystemMessage(
                        QString("Cannot upload file: %1, Couldn't convert "
                                "image to png.")
                            .arg(path.toLocalFile())));
                }
            }
            else if (path.toLocalFile().endsWith(".gif"))
            {
                channel->addMessage(makeSystemMessage(
                    QString("Uploading GIF: %1").arg(path.toLocalFile())));
                QFile file(path.toLocalFile());
                bool isOkay = file.open(QIODevice::ReadOnly);
                if (!isOkay)
                {
                    channel->addMessage(
                        makeSystemMessage(QString("Failed to open file. :(")));
                    return;
                }
                TypedBytes data = {file.readAll(), "gif"};
                uploadQueue.push(data);
                file.close();
                // file.readAll() => might be a bit big but it /should/ work
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    QString("Cannot upload file: %1, not an image")
                        .arg(path.toLocalFile())));
                isUploading = false;
            }
        }
        if (!uploadQueue.empty())
        {
            uploadImageToNuuls(uploadQueue.front(), channel, outputTextEdit);
            uploadQueue.pop();
        }
    }
    else
    {  // not PNG, try loading it into QImage and save it to a PNG.
        QImage image = qvariant_cast<QImage>(source->imageData());
        boost::optional<QByteArray> imageData = convertToPng(image);
        if (imageData)
        {
            uploadImageToNuuls({imageData.get(), "png"}, channel,
                               outputTextEdit);
        }
        else
        {
            channel->addMessage(makeSystemMessage(
                QString("Cannot upload file, failed to convert to png.")));
        }
    }
}
}  // namespace chatterino
