#pragma once

#include "widgets/dialogs/LoginDialog.hpp"

#include <QHttpServer>
#include <QTcpServer>
#include <QtHttpServer/QHttpServer>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

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

class LoginBoost : public std::enable_shared_from_this<LoginBoost>
{
public:
    LoginBoost(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection
    void start();

private:
    // The socket for the currently connected client
    tcp::socket socket_;

    // The buffer for performing reads
    beast::flat_buffer buffer_{8192};

    // The request message
    http::request<http::dynamic_body> request_;

    // The response message
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing
    net::steady_timer deadline_{socket_.get_executor(),
                                std::chrono::seconds(15)};

    // Asynchronously receive a complete request message
    void readRequest();

    // Determine what needs to be done with the request message
    void processRequest();

    // Construct a response message based on the program state
    void createResponse();

    // Asynchronously transmit the response message
    void writeResponse();

    // Check whether we have spent enough time on this connection
    void checkDeadline();
};

}  // namespace chatterino
