#include "common/NetworkRequest.hpp"

#include "common/NetworkPrivate.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"

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

NetworkRequest::NetworkRequest(const QUrl &url, NetworkRequestType requestType)
    : data(new NetworkData)
{
    this->data->request_.setUrl(url);
    this->data->requestType_ = requestType;

    this->initializeDefaultValues();
}

NetworkRequest::~NetworkRequest() = default;

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
    this->data->onReplyCreated_ = std::move(cb);
    return std::move(*this);
}

NetworkRequest NetworkRequest::onError(NetworkErrorCallback cb) &&
{
    this->data->onError_ = std::move(cb);
    return std::move(*this);
}

NetworkRequest NetworkRequest::onSuccess(NetworkSuccessCallback cb) &&
{
    this->data->onSuccess_ = std::move(cb);
    return std::move(*this);
}

NetworkRequest NetworkRequest::finally(NetworkFinallyCallback cb) &&
{
    this->data->finally_ = std::move(cb);
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

NetworkRequest NetworkRequest::header(QNetworkRequest::KnownHeaders header,
                                      const QVariant &value) &&
{
    this->data->request_.setHeader(header, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::headerList(
    const std::vector<std::pair<QByteArray, QByteArray>> &headers) &&
{
    for (const auto &[headerName, headerValue] : headers)
    {
        this->data->request_.setRawHeader(headerName, headerValue);
    }
    return std::move(*this);
}

NetworkRequest NetworkRequest::timeout(int ms) &&
{
    this->data->hasTimeout_ = true;
    this->data->timeoutMS_ = ms;
    return std::move(*this);
}

NetworkRequest NetworkRequest::concurrent() &&
{
    this->data->executeConcurrently_ = true;
    return std::move(*this);
}

NetworkRequest NetworkRequest::multiPart(QHttpMultiPart *payload) &&
{
    payload->setParent(this->data->lifetimeManager_);
    this->data->multiPartPayload_ = payload;
    return std::move(*this);
}

NetworkRequest NetworkRequest::followRedirects(bool on) &&
{
    if (on)
    {
        this->data->request_.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::NoLessSafeRedirectPolicy);
    }
    else
    {
        this->data->request_.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::ManualRedirectPolicy);
    }

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
        qCDebug(chatterinoCommon) << "Can only cache GET requests!";
        this->data->cache_ = false;
    }

    // Can not have a caller and be concurrent at the same time.
    assert(!(this->data->caller_ && this->data->executeConcurrently_));

    load(std::move(this->data));
}

void NetworkRequest::initializeDefaultValues()
{
    const auto userAgent = QStringLiteral("chatterino/%1 (%2)")
                               .arg(Version::instance().version(),
                                    Version::instance().commitHash())
                               .toUtf8();

    this->data->request_.setRawHeader("User-Agent", userAgent);
}

NetworkRequest NetworkRequest::json(const QJsonArray &root) &&
{
    return std::move(*this).json(QJsonDocument(root));
}

NetworkRequest NetworkRequest::json(const QJsonObject &root) &&
{
    return std::move(*this).json(QJsonDocument(root));
}

NetworkRequest NetworkRequest::json(const QJsonDocument &document) &&
{
    return std::move(*this).json(document.toJson(QJsonDocument::Compact));
}

NetworkRequest NetworkRequest::json(const QByteArray &payload) &&
{
    return std::move(*this)
        .payload(payload)
        .header(QNetworkRequest::ContentTypeHeader, "application/json")
        .header(QNetworkRequest::ContentLengthHeader, payload.length())
        .header("Accept", "application/json");
}

}  // namespace chatterino
