#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/imageuploader/UploadedImageModel.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QJsonObject>
#include <QMimeData>
#include <QMutex>
#include <QObject>
#include <QString>

#include <memory>
#include <queue>

namespace chatterino {

class ResizingTextEdit;
class Channel;
class NetworkResult;
using ChannelPtr = std::shared_ptr<Channel>;

struct UploadedImage {
    QString channelName;
    QString deletionLink;
    QString imageLink;
    QString localPath;
    int64_t timestamp{};

    UploadedImage() = default;
    UploadedImage(QJsonObject obj)
        : channelName(obj["channelName"].toString())
        , imageLink(obj["imageLink"].toString())
        , timestamp(obj["timestamp"].toInt())

    {
        auto del = obj["deletionLink"];
        if (!del.isNull())
        {
            this->deletionLink = del.toString();
        }
        auto path = obj["localPath"];
        if (!path.isNull())
        {
            this->localPath = path.toString();
        }
    }

    QJsonObject toJson() const
    {
        QJsonObject out;
        out["channelName"] = this->channelName;
        out["deletionLink"] = this->deletionLink.isEmpty()
                                  ? QJsonValue(QJsonValue::Null)
                                  : this->deletionLink;
        out["imageLink"] = this->imageLink;
        out["localPath"] = this->localPath.isEmpty()
                               ? QJsonValue(QJsonValue::Null)
                               : this->localPath;

        // without cast, this is ambiguous
        out["timestamp"] = (qint64)this->timestamp;
        return out;
    }
};

class UploadedImageModel;

struct RawImageData {
    QByteArray data;
    QString format;
    QString filePath;
};

class ImageUploader final : public Singleton
{
public:
    /**
     * Tries to get the image(s) from the given QMimeData
     *
     * If no images were found, the second value in the pair will contain an error message
     */
    std::pair<std::queue<RawImageData>, QString> getImages(
        const QMimeData *source) const;

    void save() override;
    void upload(std::queue<RawImageData> images, ChannelPtr channel,
                QPointer<ResizingTextEdit> outputTextEdit);
    UploadedImageModel *createModel(QObject *parent);

private:
    void sendImageUploadRequest(RawImageData imageData, ChannelPtr channel,
                                QPointer<ResizingTextEdit> textEdit);

    // This is called from the onSuccess handler of the NetworkRequest in sendImageUploadRequest
    void handleSuccessfulUpload(const NetworkResult &result,
                                QString originalFilePath, ChannelPtr channel,
                                QPointer<ResizingTextEdit> textEdit);
    void handleFailedUpload(const NetworkResult &result, ChannelPtr channel);

    void logToFile(const QString &originalFilePath, const QString &imageLink,
                   const QString &deletionLink, ChannelPtr channel);
    // These variables are only used from the main thread.
    QMutex uploadMutex_;
    std::queue<RawImageData> uploadQueue_;

    SignalVector<UploadedImage> images_;
    pajlada::Signals::SignalHolder signals_;
};
}  // namespace chatterino
