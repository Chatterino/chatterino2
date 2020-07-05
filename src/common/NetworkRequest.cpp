#include "common/NetworkRequest.hpp"

#include "common/NetworkPrivate.hpp"
#include "common/Outcome.hpp"
#include "common/Version.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Paths.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

#include <QDebug>
#include <QFile>
#include <QtConcurrent>

#include <cassert>

namespace chatterino {

NetworkRequest::NetworkRequest(const std::string &url,
                               NetworkRequestType requestType)
    : data(new NetworkData)
{
    this->data->request_.setUrl(QUrl(QString::fromStdString(url)));
    this->data->requestType_ = requestType;

    this->initializeDefaultValues();
}

NetworkRequest::NetworkRequest(QUrl url, NetworkRequestType requestType)
    : data(new NetworkData)
{
    this->data->request_.setUrl(url);
    this->data->requestType_ = requestType;

    this->initializeDefaultValues();
}

NetworkRequest::~NetworkRequest()
{
    //assert(!this->data || this->executed_);
}

NetworkRequest NetworkRequest::type(NetworkRequestType newRequestType) &&
{
    this->data->requestType_ = newRequestType;
    return std::move(*this);
}

NetworkRequest NetworkRequest::caller(const QObject *caller) &&
{
    if (caller)
    {
        // Caller must be in gui thread
        assert(caller->thread() == qApp->thread());

        this->data->caller_ = const_cast<QObject *>(caller);
        this->data->hasCaller_ = true;
    }
    return std::move(*this);
}

NetworkRequest NetworkRequest::onReplyCreated(NetworkReplyCreatedCallback cb) &&
{
    this->data->onReplyCreated_ = cb;
    return std::move(*this);
}

NetworkRequest NetworkRequest::onError(NetworkErrorCallback cb) &&
{
    this->data->onError_ = cb;
    return std::move(*this);
}

NetworkRequest NetworkRequest::onSuccess(NetworkSuccessCallback cb) &&
{
    this->data->onSuccess_ = cb;
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const char *headerName,
                                      const char *value) &&
{
    this->data->request_.setRawHeader(headerName, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const char *headerName,
                                      const QByteArray &value) &&
{
    this->data->request_.setRawHeader(headerName, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const char *headerName,
                                      const QString &value) &&
{
    this->data->request_.setRawHeader(headerName, value.toUtf8());
    return std::move(*this);
}

NetworkRequest NetworkRequest::headerList(const QStringList &headers) &&
{
    for (const QString &header : headers)
    {
        const QStringList thisHeader = header.trimmed().split(":");
        if (thisHeader.size() == 2)
        {
            this->data->request_.setRawHeader(thisHeader[0].trimmed().toUtf8(),
                                              thisHeader[1].trimmed().toUtf8());
        }
    }
    return std::move(*this);
}

NetworkRequest NetworkRequest::timeout(int ms) &&
{
    this->data->hasTimeout_ = true;
    this->data->timer_->setInterval(ms);
    return std::move(*this);
}

NetworkRequest NetworkRequest::concurrent() &&
{
    this->data->executeConcurrently_ = true;
    return std::move(*this);
}

NetworkRequest NetworkRequest::authorizeTwitchV5(const QString &clientID,
                                                 const QString &oauthToken) &&
{
    // TODO: make two overloads, with and without oauth token
    auto tmp = std::move(*this)
                   .header("Client-ID", clientID)
                   .header("Accept", "application/vnd.twitchtv.v5+json");

    if (!oauthToken.isEmpty())
        return std::move(tmp).header("Authorization", "OAuth " + oauthToken);
    else
        return tmp;
}

NetworkRequest NetworkRequest::multiPart(QHttpMultiPart *payload) &&
{
    payload->setParent(this->data->lifetimeManager_);
    this->data->multiPartPayload_ = payload;
    return std::move(*this);
}

NetworkRequest NetworkRequest::payload(const QByteArray &payload) &&
{
    this->data->payload_ = payload;
    return std::move(*this);
}

NetworkRequest NetworkRequest::cache() &&
{
    this->data->cache_ = true;
    return std::move(*this);
}

void NetworkRequest::execute()
{
    this->executed_ = true;

    // Only allow caching for GET request
    if (this->data->cache_ &&
        this->data->requestType_ != NetworkRequestType::Get)
    {
        qDebug() << "Can only cache GET requests!";
        this->data->cache_ = false;
    }

    // Can not have a caller and be concurrent at the same time.
    assert(!(this->data->caller_ && this->data->executeConcurrently_));

    load(std::move(this->data));
}

void NetworkRequest::initializeDefaultValues()
{
    const auto userAgent = QString("chatterino/%1 (%2)")
                               .arg(CHATTERINO_VERSION, CHATTERINO_GIT_HASH)
                               .toUtf8();

    this->data->request_.setRawHeader("User-Agent", userAgent);
}

// Helper creator functions
NetworkRequest NetworkRequest::twitchRequest(QUrl url)
{
    return NetworkRequest(url).authorizeTwitchV5(getDefaultClientID());
}

}  // namespace chatterino
