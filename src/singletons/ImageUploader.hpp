#pragma once

#include "common/Singleton.hpp"

#include <QMimeData>
#include <QString>

#include <memory>

namespace chatterino {

class ResizingTextEdit;
class Channel;
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
                ResizingTextEdit &outputTextEdit);

private:
    void sendImageUploadRequest(RawImageData imageData, ChannelPtr channel,
                                ResizingTextEdit &textEdit);
};
}  // namespace chatterino
