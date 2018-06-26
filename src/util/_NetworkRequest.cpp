#include "util/networkrequest.hpp"

#include "application.hpp"

namespace chatterino {
namespace util {

NetworkRequest::NetworkRequest(const char *url)
{
    this->data.request.setUrl(QUrl(url));
}

NetworkRequest::NetworkRequest(const std::string &url)
{
    this->data.request.setUrl(QUrl(QString::fromStdString(url)));
}

NetworkRequest::NetworkRequest(const QString &url)
{
    this->data.request.setUrl(QUrl(url));
}

void NetworkRequest::setUseQuickLoadCache(bool value)
{
    this->data.useQuickLoadCache = value;
}

void NetworkRequest::Data::writeToCache(const QByteArray &bytes)
{
    if (this->useQuickLoadCache) {
        auto app = getApp();

        QFile cachedFile(app->paths->cacheDirectory + "/" + this->getHash());

        if (cachedFile.open(QIODevice::WriteOnly)) {
            cachedFile.write(bytes);

            cachedFile.close();
        }
    }
}

}  // namespace util
}  // namespace chatterino
