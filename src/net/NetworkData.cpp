#include "net/NetworkData.hpp"

#include "util/DebugCount.hpp"

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>

namespace chatterino
{
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
        if (this->hash_.isEmpty())
        {
            QByteArray bytes;

            bytes.append(this->request_.url().toString());

            for (const auto& header : this->request_.rawHeaderList())
            {
                bytes.append(header);
            }

            QByteArray hashBytes(
                QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));

            this->hash_ = hashBytes.toHex();
        }

        return this->hash_;
    }

    void NetworkData::writeToCache(const QByteArray& bytes)
    {
        if (this->useQuickLoadCache_)
        {
            qDebug() << "quickload cache disabled";

            //            QFile cachedFile(
            //                getPaths()->cacheDirectory() + "/" +
            //                this->getHash());

            //            if (cachedFile.open(QIODevice::WriteOnly))
            //            {
            //                cachedFile.write(bytes);

            //                cachedFile.close();
            //            }
        }
    }

}  // namespace chatterino
