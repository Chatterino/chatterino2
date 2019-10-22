#include "common/Channel.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"

#include <QMimeData>
#include <QString>

namespace chatterino {
struct RawImageData {
    QByteArray data;
    QString format;
};
void upload(QByteArray imageData, ChannelPtr channel,
            ResizingTextEdit &textEdit, std::string format);
void upload(RawImageData imageData, ChannelPtr channel,
            ResizingTextEdit &textEdit);
void upload(const QMimeData *source, ChannelPtr channel,
            ResizingTextEdit &outputTextEdit);
}  // namespace chatterino
