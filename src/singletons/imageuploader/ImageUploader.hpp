#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "pajlada/settings/setting.hpp"
#include "pajlada/settings/settingmanager.hpp"
#include "singletons/imageuploader/UploadedImage.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QMimeData>
#include <QMutex>
#include <qobject.h>
#include <QString>

#include <memory>
#include <queue>
#include <vector>

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
class UploadedImageModel;
class ImageUploader final : public Singleton
{
public:
    void save() override;
    void upload(const QMimeData *source, ChannelPtr channel,
                QPointer<ResizingTextEdit> outputTextEdit);
    void initialize(Settings &settings, const Paths &paths) override;
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

    std::shared_ptr<pajlada::Settings::SettingManager> sm_;
    SignalVector<UploadedImage> images_;
    std::unique_ptr<pajlada::Settings::Setting<std::vector<UploadedImage>>>
        uploadedImagesSetting_;
    pajlada::Signals::SignalHolder signals_;
};
}  // namespace chatterino
