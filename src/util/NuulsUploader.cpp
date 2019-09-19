#include "NuulsUploader.hpp"

#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"

#include <boost/algorithm/string/replace.hpp>

namespace chatterino {
bool isUploading = false;

std::queue<TypedBytes> uploadQueue;
void uploadImageToNuuls(QByteArray imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit)
{
    uploadImageToNuuls(imageData, channel, textEdit, "png");
}
void uploadImageToNuuls(TypedBytes imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit)
{
    uploadImageToNuuls(imageData.data, channel, textEdit, imageData.type);
}
void uploadImageToNuuls(QByteArray imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit, std::string format)
{
    QByteArray dataToSend;
    const char *boundary = "thisistheboudaryasd";
    static QUrl url(Env::get().imagePasteSiteUrl);

    dataToSend.insert(0, "--");
    dataToSend.append(boundary);
    std::string temp = "\r\n"
                       "Content-Disposition: form-data; name=\"attachment\"; "
                       "filename=\"control_v.%\"\r\n"
                       "Content-Type: image/%\r\n"
                       "\r\n";
    boost::replace_all(temp, "%", format);

    dataToSend.append(temp.c_str());
    dataToSend.append(imageData);
    dataToSend.append("\r\n--");
    dataToSend.append(boundary);
    dataToSend.append("\r\n");

    NetworkRequest(url, NetworkRequestType::Post)
        .header("Content-Type", (std::string("multipart/form-data; boundary=") +
                                 std::string(boundary))
                                    .c_str())

        .payload(dataToSend)
        .onSuccess([&textEdit, channel](NetworkResult result) -> Outcome {
            textEdit.insertPlainText(result.getData() + QString(" "));
            //            this->input_->ui_.textEdit->insertPlainText(result.getData());
            if (uploadQueue.size())
            {
                channel->addMessage(makeSystemMessage(
                    QString("Your image has been uploaded. %1 left. Please "
                            "wait until all of them are uploaded. About %2 "
                            "seconds left.")
                        .arg(uploadQueue.size())
                        .arg(uploadQueue.size() * 3)));
                // Argument number 2 is the ETA.
                // 2 seconds for the timer that's there not to spam Nuuls' server
                // and 1 second of actual uploading.
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    QString("Your image has been uploaded.")));
            }
            isUploading = false;
            QTimer::singleShot(2000, [channel, &textEdit]() {
                if (uploadQueue.size())
                {
                    uploadImageToNuuls(uploadQueue.front(), channel, textEdit);
                    uploadQueue.pop();
                }
            });
            return Success;
        })
        .onError([channel](int error_code) -> bool {
            channel->addMessage(makeSystemMessage(
                QString("An error happened while uploading your image: %1")
                    .arg(error_code)));
            isUploading = false;
            return true;
        })
        .execute();
}
QString getImageFileFormat(QString path)
{
    static QStringList LIST_OF_IMAGE_FORMATS = {".png", ".jpg", ".jpeg"};
    for (QString i : LIST_OF_IMAGE_FORMATS)
    {
        if (path.endsWith(i))
        {
            return i.replace('.', "");
        }
    }
    return QString();
}

void pasteFromClipboard(const QMimeData *source, ChannelPtr channel,
                        ResizingTextEdit &outputTextEdit)
{
    /*
static QUrl url("http://localhost:7494/upload?password=xd");
// default port and password for nuuls' filehost.
*/
    if (isUploading)
    {
        channel->addMessage(makeSystemMessage(
            QString("You are already uploading an image. "
                    "Please wait until the upload finishes.")));
        return;
    }

    isUploading = true;
    channel->addMessage(makeSystemMessage(QString("Started upload...")));

    if (source->hasFormat("image/png"))
    {
        uploadImageToNuuls(source->data("image/png"), channel, outputTextEdit);
    }
    else if (source->hasFormat("text/uri-list"))
    {
        QStringList potientialPathsToSend =
            QString(source->data("text/uri-list").toStdString().c_str())
                .split("\r\n");

        for (QString path : potientialPathsToSend)
        {
            if (path.isEmpty())
            {
                break;
            }
            else
            {
                if (getImageFileFormat(path) != QString())
                {
                    channel->addMessage(makeSystemMessage(
                        QString("Uploading image: %1").arg(path)));
                    QImage img = QImage(QUrl(path).toLocalFile());
                    if (img.isNull())
                    {
                        channel->addMessage(makeSystemMessage(
                            QString("Couldn't load image :(")));
                        return;
                    }
                    QByteArray imageData;
                    QBuffer buf(&imageData);
                    buf.open(QIODevice::WriteOnly);
                    img.save(&buf, "png");

                    TypedBytes data = {imageData, "png"};
                    uploadQueue.push(data);
                }
                else if (path.endsWith(".gif"))
                {
                    channel->addMessage(makeSystemMessage(
                        QString("Uploading GIF: %1").arg(path)));
                    QFile file(QUrl(path).toLocalFile());
                    bool isOkay = file.open(QIODevice::ReadOnly);
                    if (!isOkay)
                    {
                        channel->addMessage(makeSystemMessage(
                            QString("Failed to open file. :(")));
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
                            .arg(path)));
                }
            }
        }
        if (uploadQueue.size())
        {
            uploadImageToNuuls(uploadQueue.front(), channel, outputTextEdit);
            uploadQueue.pop();
        }
    }
    else if (source->hasFormat("image/gif"))
    {
        TypedBytes data = {source->data("image/gif"), "gif"};
        uploadImageToNuuls(data, channel, outputTextEdit);
    }
    else
    {  // not PNG, try loading it into QImage and save it to a PNG.
        QImage image = qvariant_cast<QImage>(source->imageData());
        QByteArray imageData;
        QBuffer buf(&imageData);
        buf.open(QIODevice::WriteOnly);
        image.save(&buf, "png");

        uploadImageToNuuls(imageData, channel, outputTextEdit);
    }
}
}  // namespace chatterino
