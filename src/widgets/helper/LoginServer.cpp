#include "widgets/helper/LoginServer.hpp"

#include "common/QLogging.hpp"
#include "widgets/dialogs/LoginDialog.hpp"

#include <QFile>

namespace chatterino {

namespace {
    // zneix: This part of boost's example looks stupid as hell, I'll try not using it and see what breaks
    // "Loop" forever accepting new connections
    void httpServer(tcp::acceptor &acceptor, tcp::socket &socket)
    {
        acceptor.async_accept(socket, [&](beast::error_code ec) {
            if (!ec)
            {
                std::make_shared<LoginBoost>(std::move(socket))->start();
            }
            httpServer(acceptor, socket);
        });
    }
}  // namespace

LoginServer::LoginServer(LoginDialog *parent)
    : parent_(parent)
    , http_(new QHttpServer())
    , tcp_(new QTcpServer(this->http_))
{
    qCDebug(chatterinoWidget) << "Creating new HTTP server";
    this->http_->bind(this->tcp_);
    this->initializeRoutes();
}

QString LoginServer::getAddress()
{
    return QString("%1:%2")
        .arg(this->bind_.ip.toString())
        .arg(this->bind_.port);
}

bool LoginServer::listen()
{
    return this->tcp_->listen(this->bind_.ip, this->bind_.port);
}

void LoginServer::close()
{
    // There's no way to close QHttpServer, so we have to work with our QTcpServer instead
    qCDebug(chatterinoWidget) << "Closing TCP server bound to HTTP server";
    this->tcp_->close();
}

void LoginServer::initializeRoutes()
{
    // Redirect page containing JS script that takes token from URL fragment and calls /token
    this->http_->route(
        "/redirect", QHttpServerRequest::Method::GET,
        [](QHttpServerResponder &&resp) {
            QFile redirectPage(":/auth.html");
            redirectPage.open(QIODevice::ReadOnly);

            resp.write(redirectPage.readAll(),
                       {{"Access-Control-Allow-Origin", "*"},
                        {"Access-Control-Allow-Methods", "GET, PUT"},
                        {"Access-Control-Allow-Headers", "X-Access-Token"}},
                       QHttpServerResponder::StatusCode::Ok);
        });
    // For CORS, letting the browser know that it's fine to make a cross-origin request
    this->http_->route(
        ".*", QHttpServerRequest::Method::OPTIONS,
        [](QHttpServerResponder &&resp) {
            resp.write({{"Access-Control-Allow-Origin", "*"},
                        {"Access-Control-Allow-Methods", "GET, PUT"},
                        {"Access-Control-Allow-Headers", "X-Access-Token"}},
                       QHttpServerResponder::StatusCode::Ok);
        });
    // Endpoint called from /redirect that processes token passed in headers
    // Returns no content, but different headers indicating token's validity
    this->http_->route(
        "/token", QHttpServerRequest::Method::PUT,
        [this](const QHttpServerRequest &req, QHttpServerResponder &&resp) {
            // Access token wasn't specified
            if (!req.headers().contains("X-Access-Token"))
            {
                resp.write(QHttpServerResponder::StatusCode::BadRequest);
                return;
            }

            // Validate token
            const auto token = req.headers().value("X-Access-Token").toString();

            auto respPtr =
                std::make_shared<QHttpServerResponder>(std::move(resp));
            this->parent_->ui_.loginButton.setText(VALIDATING_TOKEN);
            this->parent_->logInWithToken(
                token,
                [respPtr] {
                    respPtr->write(
                        {{"Access-Control-Allow-Origin", "*"},
                         {"Access-Control-Allow-Methods", "GET, PUT"},
                         {"Access-Control-Allow-Headers", "X-Access-Token"}},
                        QHttpServerResponder::StatusCode::Ok);
                },
                [respPtr] {
                    respPtr->write(
                        {{"Access-Control-Allow-Origin", "*"},
                         {"Access-Control-Allow-Methods", "GET, PUT"},
                         {"Access-Control-Allow-Headers", "X-Access-Token"}},
                        QHttpServerResponder::StatusCode::BadRequest);
                },
                [this] {
                    this->close();
                    this->parent_->activateLoginButton();
                });
        });
}

void LoginBoost::start()
{
    readRequest();
    checkDeadline();
}

void LoginBoost::readRequest()
{
    auto self = shared_from_this();

    http::async_read(socket_, buffer_, request_,
                     [self](beast::error_code ec, size_t bytes_transferred) {
                         boost::ignore_unused(bytes_transferred);
                         if (!ec)
                         {
                             self->processRequest();
                         }
                     });
}

void LoginBoost::processRequest()
{
    response_.version(request_.version());
    response_.keep_alive(false);

    switch (request_.method())
    {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            response_.set("ssds", "asdfas");
            createResponse();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string()) << "'";
            break;
    }

    writeResponse();
}

void LoginBoost::createResponse()
{
    if (request_.target() == "/count")
    {
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body())
            << "<html>\n"
            << "<head><title>Request count</title></head>\n"
            << "<body>\n"
            << "<h1>Request count</h1>\n"
            << "<p>There have been 0 requests so far.</p>\n"
            << "</body>\n"
            << "</html>\n";
    }
    else if (request_.target() == "/time")
    {
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body())
            << "<html>\n"
            << "<head><title>Current time</title></head>\n"
            << "<body>\n"
            << "<h1>Current time</h1>\n"
            << "<p>The current time is "
            << QTime::currentTime().toString().toStdString()
            << " seconds since the epoch.</p>\n"
            << "</body>\n"
            << "</html>\n";
    }
    else
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "File not found\r\n";
    }
}

void LoginBoost::writeResponse()
{
    auto self = shared_from_this();

    response_.content_length(response_.body().size());

    http::async_write(
        socket_, response_, [self](beast::error_code ec, std::size_t) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            self->deadline_.cancel();
        });
}

void LoginBoost::checkDeadline()
{
    auto self = shared_from_this();

    deadline_.async_wait([this, self](beast::error_code ec) {
        if (!ec)
        {
            // Close socket to cancel any outstanding operation
            self->socket_.close(ec);
            this->socket_.close(ec);
        }
    });
}

}  // namespace chatterino
