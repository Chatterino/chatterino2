#include "common/NetworkData.hpp"

#include "Application.hpp"
#include "singletons/Paths.hpp"
#include "util/DebugCount.hpp"

#include <QCryptographicHash>
#include <QFile>

namespace chatterino {

NetworkData::NetworkData()
{
    DebugCount::increase("NetworkData");
}

NetworkData::~NetworkData()
{
    DebugCount::decrease("NetworkData");
}

QString NetworkData::getHash()
{
    if (this->hash_.isEmpty()) {
        QByteArray bytes;

        bytes.append(this->request_.url().toString());

        for (const auto &header : this->request_.rawHeaderList()) {
            bytes.append(header);
        }

        QByteArray hashBytes(
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));

        this->hash_ = hashBytes.toHex();
    }

    return this->hash_;
}

void NetworkData::writeToCache(const QByteArray &bytes)
{
    if (this->useQuickLoadCache_) {
        auto app = getApp();

        QFile cachedFile(app->paths->cacheDirectory + "/" + this->getHash());

        if (cachedFile.open(QIODevice::WriteOnly)) {
            cachedFile.write(bytes);

            cachedFile.close();
        }
    }
}

}  // namespace chatterino
