#include "common/network/NetworkRequest.hpp"

#include "common/network/NetworkPrivate.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QtConcurrent>

#include <cassert>

namespace chatterino {

NetworkRequest::NetworkRequest(const std::string &url,
                               NetworkRequestType requestType)
    : data(new NetworkData)
{
    this->data->request.setUrl(QUrl(QString::fromStdString(url)));
    this->data->requestType = requestType;

    this->initializeDefaultValues();
}

NetworkRequest::NetworkRequest(const QUrl &url, NetworkRequestType requestType)
    : data(new NetworkData)
{
    this->data->request.setUrl(url);
    this->data->requestType = requestType;

    this->initializeDefaultValues();
}

NetworkRequest::~NetworkRequest() = default;

NetworkRequest NetworkRequest::type(NetworkRequestType newRequestType) &&
{
    this->data->requestType = newRequestType;
    return std::move(*this);
}

NetworkRequest NetworkRequest::caller(const QObject *caller) &&
{
    if (caller)
    {
        // Caller must be in gui thread
        assert(caller->thread() == QApplication::instance()->thread());

        this->data->caller = const_cast<QObject *>(caller);
        this->data->hasCaller = true;
    }
    return std::move(*this);
}

NetworkRequest NetworkRequest::onError(NetworkErrorCallback cb) &&
{
    this->data->onError = std::move(cb);
    return std::move(*this);
}

NetworkRequest NetworkRequest::onSuccess(NetworkSuccessCallback cb) &&
{
    this->data->onSuccess = std::move(cb);
    return std::move(*this);
}

NetworkRequest NetworkRequest::finally(NetworkFinallyCallback cb) &&
{
    this->data->finally = std::move(cb);
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const char *headerName,
                                      const char *value) &&
{
    this->data->request.setRawHeader(headerName, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const char *headerName,
                                      const QByteArray &value) &&
{
    this->data->request.setRawHeader(headerName, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const char *headerName,
                                      const QString &value) &&
{
    this->data->request.setRawHeader(headerName, value.toUtf8());
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(QNetworkRequest::KnownHeaders header,
                                      const QVariant &value) &&
{
    this->data->request.setHeader(header, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::header(const QByteArray &headerName,
                                      const QByteArray &value) &&
{
    this->data->request.setRawHeader(headerName, value);
    return std::move(*this);
}

NetworkRequest NetworkRequest::headerList(
    const std::vector<std::pair<QByteArray, QByteArray>> &headers) &&
{
    for (const auto &[headerName, headerValue] : headers)
    {
        this->data->request.setRawHeader(headerName, headerValue);
    }
    return std::move(*this);
}

NetworkRequest NetworkRequest::timeout(int ms) &&
{
    this->data->timeout = std::chrono::milliseconds(ms);
    return std::move(*this);
}

NetworkRequest NetworkRequest::concurrent() &&
{
    this->data->executeConcurrently = true;
    return std::move(*this);
}

NetworkRequest NetworkRequest::multiPart(QHttpMultiPart *payload) &&
{
    this->data->multiPartPayload = {payload, {}};
    return std::move(*this);
}

NetworkRequest NetworkRequest::followRedirects(bool on) &&
{
    if (on)
    {
        this->data->request.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::NoLessSafeRedirectPolicy);
    }
    else
    {
        this->data->request.setAttribute(
            QNetworkRequest::RedirectPolicyAttribute,
            QNetworkRequest::ManualRedirectPolicy);
    }

    return std::move(*this);
}

NetworkRequest NetworkRequest::payload(const QByteArray &payload) &&
{
    this->data->payload = payload;
    return std::move(*this);
}

NetworkRequest NetworkRequest::cache() &&
{
    this->data->cache = true;
    return std::move(*this);
}

void NetworkRequest::execute()
{
    this->executed_ = true;

    // Only allow caching for GET request
    if (this->data->cache && this->data->requestType != NetworkRequestType::Get)
    {
        qCDebug(chatterinoCommon) << "Can only cache GET requests!";
        this->data->cache = false;
    }

    // Can not have a caller and be concurrent at the same time.
    assert(!(this->data->caller && this->data->executeConcurrently));

    load(std::move(this->data));
}

void NetworkRequest::initializeDefaultValues()
{
    const auto userAgent = QStringLiteral("chatterino/%1 (%2)")
                               .arg(Version::instance().version(),
                                    Version::instance().commitHash())
                               .toUtf8();

    this->data->request.setRawHeader("User-Agent", userAgent);
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

#ifndef NDEBUG
NetworkRequest NetworkRequest::ignoreSslErrors(bool ignore) &&
{
    this->data->ignoreSslErrors = ignore;
    return std::move(*this);
}
#endif

}  // namespace chatterino
