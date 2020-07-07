#pragma once

#include "common/NetworkCommon.hpp"
#include "common/NetworkResult.hpp"

#include <QHttpMultiPart>
#include <memory>

namespace chatterino {

struct NetworkData;

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
    explicit NetworkRequest(
        QUrl url, NetworkRequestType requestType = NetworkRequestType::Get);

    // Enable move
    NetworkRequest(NetworkRequest &&other) = default;
    NetworkRequest &operator=(NetworkRequest &&other) = default;

    // Disable copy
    NetworkRequest(const NetworkRequest &other) = delete;
    NetworkRequest &operator=(const NetworkRequest &other) = delete;

    ~NetworkRequest();

    NetworkRequest type(NetworkRequestType newRequestType) &&;

    NetworkRequest onReplyCreated(NetworkReplyCreatedCallback cb) &&;
    NetworkRequest onError(NetworkErrorCallback cb) &&;
    NetworkRequest onSuccess(NetworkSuccessCallback cb) &&;

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
    NetworkRequest headerList(const QStringList &headers) &&;
    NetworkRequest timeout(int ms) &&;
    NetworkRequest concurrent() &&;
    NetworkRequest authorizeTwitchV5(const QString &clientID,
                                     const QString &oauthToken = QString()) &&;
    NetworkRequest multiPart(QHttpMultiPart *payload) &&;

    void execute();

    static NetworkRequest twitchRequest(QUrl url);

private:
    void initializeDefaultValues();
};

}  // namespace chatterino
