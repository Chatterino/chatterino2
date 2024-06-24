#include "util/LoadPixmap.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QLoggingCategory>
#include <QPixmap>

namespace chatterino {

void loadPixmapFromUrl(const Url &url, std::function<void(QPixmap)> &&onSuccess,
                       const std::optional<std::function<void()>> &onError)
{
    NetworkRequest(url.string)
        .concurrent()
        .cache()
        .onError([onError](const NetworkResult & /*result*/) {
            if (onError.has_value())
            {
                (*onError)();
            }
        })
        .onSuccess([callback = std::move(onSuccess), onError,
                    url](const NetworkResult &result) {
            auto data = result.getData();
            QBuffer buffer(&data);
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer);

            if (!reader.canRead() || reader.size().isEmpty())
            {
                qCWarning(chatterinoImage)
                    << "Can't read image file at" << url.string << ":"
                    << reader.errorString();
                if (onError.has_value())
                {
                    (*onError)();
                }
                return;
            }

            QImage image = reader.read();
            if (image.isNull())
            {
                qCWarning(chatterinoImage)
                    << "Failed reading image at" << url.string << ":"
                    << reader.errorString();
                if (onError.has_value())
                {
                    (*onError)();
                }
                return;
            }

            callback(QPixmap::fromImage(image));
        })
        .execute();
}

}  // namespace chatterino
