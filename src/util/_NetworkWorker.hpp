#pragma once

#include <QObject>

class QNetworkReply;

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
