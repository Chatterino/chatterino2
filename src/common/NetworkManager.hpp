#pragma once

#include "common/NetworkRequester.hpp"
#include "common/NetworkWorker.hpp"
#include "debug/Log.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace chatterino {

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    static QThread workerThread;
    static QNetworkAccessManager accessManager;

    static void init();
    static void deinit();
};

}  // namespace chatterino
