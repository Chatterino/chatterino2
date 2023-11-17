#pragma once

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

void upload(const QMimeData *source, ChannelPtr channel,
            ResizingTextEdit &outputTextEdit);

}  // namespace chatterino
