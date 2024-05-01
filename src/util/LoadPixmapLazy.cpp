#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/Image.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QLoggingCategory>
#include <QPixmap>

namespace chatterino {

void loadPixmapFromUrlLazy(const Url &url,
                           std::function<void(QPixmap)> &&callback)
{
    NetworkRequest(url.string)
        .concurrent()
        .cache()
        .onSuccess(
            [callback = std::move(callback), url](const NetworkResult &result) {
                auto data = result.getData();

                // const cast since we are only reading from it
                QBuffer buffer(const_cast<QByteArray *>(&data));
                buffer.open(QIODevice::ReadOnly);
                QImageReader reader(&buffer);

                if (!reader.canRead() || reader.size().isEmpty())
                {
                    qCWarning(chatterinoSettings)
                        << "Can't read mod action image at" << url.string << ":"
                        << reader.errorString();
                    return;
                }

                QImage image = reader.read();
                if (image.isNull())
                {
                    qCWarning(chatterinoSettings)
                        << "Failed reading mod action image at" << url.string
                        << ":" << reader.errorString();
                    return;
                }

                callback(QPixmap::fromImage(image));
            })
        .execute();
}
}  // namespace chatterino
