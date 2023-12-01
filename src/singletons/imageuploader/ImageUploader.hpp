#pragma once

#include "common/Singleton.hpp"
#include "pajlada/settings/setting.hpp"
#include "pajlada/settings/settingmanager.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QMimeData>
#include <QMutex>
#include <QString>
#include <Qt>

#include <cstdint>
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

struct UploadedImage {
    QString channelName;
    QString deletionLink;
    QString imageLink;
    QString localPath;
    int64_t timestamp{};
};

class ImageUploader final : public Singleton
{
public:
    void save() override;
    void upload(const QMimeData *source, ChannelPtr channel,
                QPointer<ResizingTextEdit> outputTextEdit);
    void initialize(Settings &settings, Paths &paths) override;

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
    std::unique_ptr<pajlada::Settings::Setting<std::vector<UploadedImage>>>
        imageLogSetting_;
};
}  // namespace chatterino
   //
namespace pajlada {
template <>
struct Serialize<chatterino::UploadedImage> {
    static rapidjson::Value get(const chatterino::UploadedImage &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "channelName", value.channelName, a);
        chatterino::rj::set(ret, "imageLink", value.imageLink, a);
        chatterino::rj::set(ret, "timestamp", value.timestamp, a);
        chatterino::rj::set(ret, "localPath", value.localPath, a);
        chatterino::rj::set(ret, "deletionLink", value.deletionLink, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::UploadedImage> {
    static chatterino::UploadedImage get(const rapidjson::Value &value,
                                         bool *error = nullptr)
    {
        chatterino::UploadedImage img;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }

        if (value["localPath"].IsNull())
        {
            img.localPath = QString();
        }
        else if (!chatterino::rj::getSafe(value, "localPath", img.localPath))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (!chatterino::rj::getSafe(value, "imageLink", img.imageLink))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (value["deletionLink"].IsNull())
        {
            img.deletionLink = QString();
        }
        else if (!chatterino::rj::getSafe(value, "deletionLink",
                                          img.deletionLink))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (!chatterino::rj::getSafe(value, "channelName", img.channelName))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (!chatterino::rj::getSafe(value, "timestamp", img.timestamp))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }

        return img;
    }
};
}  // namespace pajlada
