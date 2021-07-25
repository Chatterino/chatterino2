#pragma once

#include "widgets/dialogs/LoginDialog.hpp"

#include <QHttpServer>
#include <QTcpServer>
#include <QtHttpServer/QHttpServer>

namespace chatterino {

class LoginDialog;

class LoginServer
{
public:
    LoginServer(LoginDialog *parent);

    QString getAddress();
    bool listen();
    void close();

private:
    LoginDialog *parent_;

    const struct {
        QHostAddress ip = QHostAddress::LocalHost;
        int port = 52107;
    } bind_;

    QHttpServer *http_;
    QTcpServer *tcp_;

    void initializeRoutes();
};

}  // namespace chatterino
