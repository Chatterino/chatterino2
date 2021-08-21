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
    LoginServer(LoginDialog *parent, tcp::socket *socket);

    // Initiate the asynchronous operations associated with the connection
    void start();

    // FeelsGoodMan
    void loop(net::io_context *iocontext, tcp::acceptor *acceptor,
              tcp::socket *socket);

    // Stops the HTTP server or something, idk I'm not the engineer
    void stop(net::io_context *iocontext);

private:
    // Parent LoginDialog instance
    LoginDialog *parent_;

    // Socket for the currently connected client
    tcp::socket *socket_;

    // Buffer for performing reads
    beast::flat_buffer buffer_{8192};

    // Request and response messages
    http::request<http::dynamic_body> request_;
    http::response<http::dynamic_body> response_;

    // Timer for putting a deadline on connection processing
    net::steady_timer deadline_{socket_->get_executor(),
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
