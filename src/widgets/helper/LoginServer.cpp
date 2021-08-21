#include "widgets/helper/LoginServer.hpp"

#include "common/QLogging.hpp"
#include "widgets/dialogs/LoginDialog.hpp"

#include <QFile>

namespace chatterino {

//LoginServer::LoginServer(LoginDialog *parent)
//    : parent_(parent)
//    , http_(new QHttpServer())
//    , tcp_(new QTcpServer(this->http_))
//{
//    qCDebug(chatterinoWidget) << "Creating new HTTP server";
//    this->http_->bind(this->tcp_);
//    this->initializeRoutes();
//}

//QString LoginServer::getAddress()
//{
//    return QString("%1:%2")
//        .arg(this->bind_.ip.toString())
//        .arg(this->bind_.port);
//}

//bool LoginServer::listen()
//{
//    return this->tcp_->listen(this->bind_.ip, this->bind_.port);
//}

//void LoginServer::close()
//{
//    // There's no way to close QHttpServer, so we have to work with our QTcpServer instead
//    qCDebug(chatterinoWidget) << "Closing TCP server bound to HTTP server";
//    this->tcp_->close();
//}

//void LoginServer::initializeRoutes()
//{
//    // Redirect page containing JS script that takes token from URL fragment and calls /token
//    this->http_->route(
//        "/redirect", QHttpServerRequest::Method::GET,
//        [](QHttpServerResponder &&resp) {
//            QFile redirectPage(":/auth.html");
//            redirectPage.open(QIODevice::ReadOnly);

//            resp.write(redirectPage.readAll(),
//                       {{"Access-Control-Allow-Origin", "*"},
//                        {"Access-Control-Allow-Methods", "GET, PUT"},
//                        {"Access-Control-Allow-Headers", "X-Access-Token"}},
//                       QHttpServerResponder::StatusCode::Ok);
//        });
//    // For CORS, letting the browser know that it's fine to make a cross-origin request
//    this->http_->route(
//        ".*", QHttpServerRequest::Method::OPTIONS,
//        [](QHttpServerResponder &&resp) {
//            resp.write({{"Access-Control-Allow-Origin", "*"},
//                        {"Access-Control-Allow-Methods", "GET, PUT"},
//                        {"Access-Control-Allow-Headers", "X-Access-Token"}},
//                       QHttpServerResponder::StatusCode::Ok);
//        });
//    // Endpoint called from /redirect that processes token passed in headers
//    // Returns no content, but different headers indicating token's validity
//    this->http_->route(
//        "/token", QHttpServerRequest::Method::PUT,
//        [this](const QHttpServerRequest &req, QHttpServerResponder &&resp) {
//            // Access token wasn't specified
//            if (!req.headers().contains("X-Access-Token"))
//            {
//                resp.write(QHttpServerResponder::StatusCode::BadRequest);
//                return;
//            }

//            // Validate token
//            const auto token = req.headers().value("X-Access-Token").toString();

//            auto respPtr =
//                std::make_shared<QHttpServerResponder>(std::move(resp));
//            this->parent_->ui_.loginButton.setText(VALIDATING_TOKEN);
//            this->parent_->logInWithToken(
//                token,
//                [respPtr] {
//                    respPtr->write(
//                        {{"Access-Control-Allow-Origin", "*"},
//                         {"Access-Control-Allow-Methods", "GET, PUT"},
//                         {"Access-Control-Allow-Headers", "X-Access-Token"}},
//                        QHttpServerResponder::StatusCode::Ok);
//                },
//                [respPtr] {
//                    respPtr->write(
//                        {{"Access-Control-Allow-Origin", "*"},
//                         {"Access-Control-Allow-Methods", "GET, PUT"},
//                         {"Access-Control-Allow-Headers", "X-Access-Token"}},
//                        QHttpServerResponder::StatusCode::BadRequest);
//                },
//                [this] {
//                    this->close();
//                    this->parent_->activateLoginButton();
//                });
//        });
//}

/// BOOST

LoginServer::LoginServer(LoginDialog *parent, tcp::socket *socket)
    : socket_(socket)
{
    qCDebug(chatterinoWidget) << "Creating new HTTP server";
}

void LoginServer::start()
{
    this->readRequest();
    this->checkDeadline();
}

void LoginServer::loop(net::io_context *iocontext, tcp::acceptor *acceptor,
                       tcp::socket *socket)
{
    qDebug() << iocontext->stopped();
    acceptor->async_accept(*this->socket_, [&](beast::error_code ec) {
        if (!ec)
        {
            std::make_shared<LoginServer>(this->parent_, socket)->start();
        }
        qDebug() << "dank";
        //        this->loop();
    });
}

void LoginServer::stop(net::io_context *iocontext)
{
    qCDebug(chatterinoWidget) << "Stopping the HTTP server";
    iocontext->stop();
}

void LoginServer::readRequest()
{
    auto self = shared_from_this();

    http::async_read(*self->socket_, buffer_, request_,
                     [self](beast::error_code ec, size_t bytes_transferred) {
                         boost::ignore_unused(bytes_transferred);
                         if (!ec)
                         {
                             self->processRequest();
                         }
                     });
}

void LoginServer::processRequest()
{
    response_.version(request_.version());
    response_.keep_alive(false);

    auto method = request_.method();
    auto target = request_.target();

    // Common headers
    response_.set(http::field::server, "Chatterino");
    response_.set(http::field::access_control_allow_origin, "*");
    response_.set(http::field::access_control_allow_methods, "GET, PUT");
    response_.set(http::field::access_control_allow_headers, "X-Access-Token");

    // GET /redirect
    if (method == http::verb::get && target == "/redirect")
    {
        // Read auth.html from resources/
        QFile redirectPageFile(":/auth.html");
        redirectPageFile.open(QIODevice::ReadOnly);
        auto redirectPage = redirectPageFile.readAll().toStdString();

        // Write successful response
        response_.result(http::status::ok);
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body()) << redirectPage;
    }
    // OPTIONS .*
    else if (method == http::verb::options)
    {
        // Respond with 200 for CORS calls
        response_.result(http::status::ok);
    }
    // PUT /token
    else if (method == http::verb::put && target == "/token")
    {
        // Handle received token
        auto it = request_.find("X-Access-Token");
        if (it == request_.end())
        {
            qDebug() << "X-Access-Token wasn't found!";
            response_.result(http::status::bad_request);
        }
        else
        {
            // TODO: reimplement zulu magic for validating the token etc.
            qDebug() << it->value().to_string().data();
            response_.result(http::status::ok);
        }
    }
    // Unhandled route
    else
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");

        beast::ostream(response_.body()) << "404 Not found";
    }

    // Send the response to the client
    this->writeResponse();
}

void LoginServer::writeResponse()
{
    auto self = shared_from_this();

    response_.content_length(response_.body().size());

    http::async_write(
        *self->socket_, response_, [self](beast::error_code ec, std::size_t) {
            self->socket_->shutdown(tcp::socket::shutdown_send, ec);
            self->deadline_.cancel();
        });
}

void LoginServer::checkDeadline()
{
    auto self = shared_from_this();

    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec)
        {
            // Close socket to cancel any outstanding operation
            self->socket_->close(ec);
        }
    });
}

}  // namespace chatterino
