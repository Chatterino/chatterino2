#pragma once

#include "widgets/dialogs/LoginDialog.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace chatterino {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

//class LoginServer
//{
//public:
//    LoginServer(LoginDialog *parent);

//    QString getAddress();
//    bool listen();
//    void close();

//private:
//    LoginDialog *parent_;

//    const struct {
//        QHostAddress ip = QHostAddress::LocalHost;
//        int port = 52107;
//    } bind_;

//    QHttpServer *http_;
//    QTcpServer *tcp_;

//    void initializeRoutes();
//};

class LoginDialog;

class LoginServer : public std::enable_shared_from_this<LoginServer>
{
public:
    LoginServer(LoginDialog *parent, net::io_context &ioContext);

    // Kickstarts the acceptor into accepting new sessions
    void start();

    // Stops the acceptor from accepting new sessions
    void stop();

    unsigned short port_ = 52107;

private:
    // doAccept starts listening to the acceptor
    void doAccept();

    // onAccept is called when a client has been accepted by the accptor
    void onAccept(beast::error_code ec, tcp::socket socket);

    net::io_context &ioContext_;

    // Acceptor
    tcp::acceptor acceptor_;

    // Parent LoginDialog instance
    LoginDialog *parent_;
};

}  // namespace chatterino
