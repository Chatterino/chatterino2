// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QNetworkAccessManager>
#include <QThread>

namespace chatterino {

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    static QThread *workerThread;
    static QNetworkAccessManager *accessManager;

    static void init();
    static void deinit();
};

}  // namespace chatterino
