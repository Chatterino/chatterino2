#include <QMimeData>
#include <QString>
#include "common/Channel.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"

namespace chatterino {
struct TypedBytes {
    QByteArray data;
    std::string type;
};
void uploadImageToNuuls(QByteArray imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit, std::string format);
void uploadImageToNuuls(TypedBytes imageData, ChannelPtr channel,
                        ResizingTextEdit &textEdit);
QString getImageFileFormat(QString path);
void pasteFromClipboard(const QMimeData *source, ChannelPtr channel,
                        ResizingTextEdit &outputTextEdit);
}  // namespace chatterino
