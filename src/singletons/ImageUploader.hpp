#pragma once

#include "common/Singleton.hpp"

#include <QMimeData>
#include <QMutex>
#include <QString>

#include <memory>
#include <queue>

namespace chatterino {

class ResizingTextEdit;
class Channel;
class NetworkResult;
using ChannelPtr = std::shared_ptr<Channel>;

struct RawImageData {
    QByteArray data;
    QString format;
    QString filePath;
};

class ImageUploader final : public Singleton
{
public:
    void save() override;
    void upload(const QMimeData *source, ChannelPtr channel,
                QPointer<ResizingTextEdit> outputTextEdit);

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
};
}  // namespace chatterino
