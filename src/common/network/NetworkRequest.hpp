#pragma once

#include "common/network/NetworkCommon.hpp"

#include <QHttpMultiPart>

#include <memory>

class QJsonArray;
class QJsonObject;
class QJsonDocument;

namespace chatterino {

class NetworkData;

class NetworkRequest final
{
    // Stores all data about the request that needs to be passed around to each
    // part of the request
    std::shared_ptr<NetworkData> data;

    // The NetworkRequest destructor will assert if executed_ hasn't been set to
    // true before dying
    bool executed_ = false;

public:
    explicit NetworkRequest(
        const std::string &url,
        NetworkRequestType requestType = NetworkRequestType::Get);
    explicit NetworkRequest(const QUrl &url, NetworkRequestType requestType =
                                                 NetworkRequestType::Get);

    // Enable move
    NetworkRequest(NetworkRequest &&other) = default;
    NetworkRequest &operator=(NetworkRequest &&other) = default;

    // Disable copy
    NetworkRequest(const NetworkRequest &other) = delete;
    NetworkRequest &operator=(const NetworkRequest &other) = delete;

    ~NetworkRequest();

    NetworkRequest type(NetworkRequestType newRequestType) &&;

    NetworkRequest onError(NetworkErrorCallback cb) &&;
    NetworkRequest onSuccess(NetworkSuccessCallback cb) &&;
    NetworkRequest finally(NetworkFinallyCallback cb) &&;

    NetworkRequest payload(const QByteArray &payload) &&;
    NetworkRequest cache() &&;
    /// NetworkRequest makes sure that the `caller` object still exists when the
    /// callbacks are executed. Cannot be used with concurrent() since we can't
    /// make sure that the object doesn't get deleted while the callback is
    /// running.
    NetworkRequest caller(const QObject *caller) &&;
    NetworkRequest header(const char *headerName, const char *value) &&;
    NetworkRequest header(const char *headerName, const QByteArray &value) &&;
    NetworkRequest header(const char *headerName, const QString &value) &&;
    NetworkRequest header(const QByteArray &headerName,
                          const QByteArray &value) &&;
    NetworkRequest header(QNetworkRequest::KnownHeaders header,
                          const QVariant &value) &&;
    NetworkRequest headerList(
        const std::vector<std::pair<QByteArray, QByteArray>> &headers) &&;
    NetworkRequest timeout(int ms) &&;
    NetworkRequest concurrent() &&;
    NetworkRequest multiPart(QHttpMultiPart *payload) &&;
    /**
     * This will change `RedirectPolicyAttribute`.
     * `QNetworkRequest`'s defaults are used by default (Qt 5: no-follow, Qt 6: follow).
     */
    NetworkRequest followRedirects(bool on) &&;
    NetworkRequest json(const QJsonObject &root) &&;
    NetworkRequest json(const QJsonArray &root) &&;
    NetworkRequest json(const QJsonDocument &document) &&;
    NetworkRequest json(const QByteArray &payload) &&;

#ifndef NDEBUG
    NetworkRequest ignoreSslErrors(bool ignore) &&;
#endif

    void execute();

private:
    void initializeDefaultValues();
};

}  // namespace chatterino
