#include "common/Channel.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"

#include <QMimeData>
#include <QString>

namespace chatterino {
struct TypedBytes {
    QByteArray data;
    QString type;
};
void upload(QByteArray imageData, ChannelPtr channel,
            ResizingTextEdit &textEdit, std::string format);
void upload(TypedBytes imageData, ChannelPtr channel,
            ResizingTextEdit &textEdit);
void upload(const QMimeData *source, ChannelPtr channel,
            ResizingTextEdit &outputTextEdit);
}  // namespace chatterino

namespace {
QString getImageFileFormat(QString path);
}
