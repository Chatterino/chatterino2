#pragma once

#include <QObject>

class QNetworkReply;

namespace chatterino {

class NetworkWorker : public QObject
{
    Q_OBJECT

signals:
    void doneUrl(QNetworkReply *);
};

}  // namespace chatterino
