#include "widgets/helper/LoginServer.hpp"

#include "common/QLogging.hpp"
#include "widgets/dialogs/LoginDialog.hpp"

#include <QFile>

namespace chatterino {

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

}  // namespace chatterino
