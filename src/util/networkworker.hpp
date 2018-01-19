#pragma once

#include <QObject>

namespace chatterino {
namespace util {

class NetworkWorker : public QObject
{
    Q_OBJECT

signals:
    void doneUrl(QNetworkReply *);
};

}  // namespace util
}  // namespace chatterino
