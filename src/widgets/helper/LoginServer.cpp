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

class LoginServerSession
    : public std::enable_shared_from_this<LoginServerSession>
{
public:
    LoginServerSession(tcp::socket &&socket)
        : stream_(std::move(socket))
        , lambda_(*this)
    {
    }

    void run()
    {
        net::dispatch(this->stream_.get_executor(),
                      beast::bind_front_handler(&LoginServerSession::doRead,
                                                shared_from_this()));
    }

    void doRead()
    {
        this->request_ = {};
        this->stream_.expires_after(std::chrono::seconds(15));

        http::async_read(this->stream_, this->buffer_, this->request_,
                         beast::bind_front_handler(&LoginServerSession::onRead,
                                                   shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t /*bytesTransferred*/)
    {
        if (ec)
        {
            if (ec == http::error::end_of_stream)
            {
                // nice close
                this->doClose();
                return;
            }

            // non-nice close
            qDebug() << "unhandled error in onRead:" << ec.message().c_str();
            return;
        }

        this->handleRequest(std::move(this->request_));
    }

    void doClose()
    {
        beast::error_code ec;
        this->stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    void handleRequest(http::request<http::string_body> request)
    {
        http::response<http::dynamic_body> response;

        response.version(request.version());
        response.keep_alive(false);

        auto method = request.method();
        auto target = request.target();

        // Common headers
        response.set(http::field::server, "Chatterino");
        response.set(http::field::access_control_allow_origin, "*");
        response.set(http::field::access_control_allow_methods, "GET, PUT");
        response.set(http::field::access_control_allow_headers,
                     "X-Access-Token");

        // GET /redirect
        if (method == http::verb::get && target == "/redirect")
        {
            // Read auth.html from resources/
            QFile redirectPageFile(":/auth.html");
            redirectPageFile.open(QIODevice::ReadOnly);
            auto redirectPage = redirectPageFile.readAll().toStdString();

            // Write successful response
            response.result(http::status::ok);
            response.set(http::field::content_type, "text/html");
            beast::ostream(response.body()) << redirectPage;
        }
        // OPTIONS .*
        else if (method == http::verb::options)
        {
            // Respond with 200 for CORS calls
            response.result(http::status::ok);
        }
        // PUT /token
        else if (method == http::verb::put && target == "/token")
        {
            // Handle received token
            auto it = request.find("X-Access-Token");
            if (it == request.end())
            {
                qDebug() << "X-Access-Token wasn't found!";
                response.result(http::status::bad_request);
            }
            else
            {
                // TODO: reimplement zulu magic for validating the token etc.
                qDebug() << it->value().to_string().data();
                response.result(http::status::ok);
            }
        }
        // Unhandled route
        else
        {
            response.result(http::status::not_found);
            response.set(http::field::content_type, "text/plain");

            beast::ostream(response.body()) << "404 Not found";
        }

        response.content_length(response.body().size());

        this->lambda_(std::move(response));
        // http::async_write(
        //     this->stream_, response,
        //     beast::bind_front_handler(&LoginServerSession::onWrite,
        //                               shared_from_this()));
    }

    void onWrite(bool close, beast::error_code ec,
                 std::size_t /*bytesTransferred*/)
    {
        if (ec)
        {
            qDebug() << "error in onWrite: " << ec.message().c_str();
            return;
        }

        assert(close);

        if (close)
        {
            this->doClose();
        }
        else
        {
            qDebug() << "NO CLOSE?????";
        }
    }

private:
    struct SendLambda {
        LoginServerSession &self_;

        explicit SendLambda(LoginServerSession &self)
            : self_(self)
        {
        }

        template <bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields> &&msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(
                std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            // Write the response
            http::async_write(self_.stream_, *sp,
                              beast::bind_front_handler(
                                  &LoginServerSession::onWrite,
                                  self_.shared_from_this(), sp->need_eof()));
        }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    SendLambda lambda_;
    std::shared_ptr<void> res_;
};

LoginServer::LoginServer(LoginDialog *parent, net::io_context &ioContext)
    : ioContext_(ioContext)
    , acceptor_(this->ioContext_,
                {net::ip::make_address("127.0.0.1"), this->port_})
    , parent_(parent)
{
    qCDebug(chatterinoWidget) << "Creating new HTTP server";
}

void LoginServer::start()
{
    this->doAccept();
}

void LoginServer::stop()
{
    qCDebug(chatterinoWidget) << "Stopping the HTTP server";
    this->acceptor_.close();
}

void LoginServer::doAccept()
{
    this->acceptor_.async_accept(
        net::make_strand(this->ioContext_),
        beast::bind_front_handler(&LoginServer::onAccept, shared_from_this()));
}

void LoginServer::onAccept(beast::error_code ec, tcp::socket socket)
{
    if (ec)
    {
        qDebug() << "error in onaccept:" << ec.message().c_str();
    }
    else
    {
        // Move socket to LoginServerSession to handle the request
        std::make_shared<LoginServerSession>(std::move(socket))->run();
    }

    // Accept further requests
    this->doAccept();
}

}  // namespace chatterino
