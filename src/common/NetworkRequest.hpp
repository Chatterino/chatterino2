#pragma once

#include "common/NetworkCommon.hpp"
#include "common/NetworkRequester.hpp"
#include "common/NetworkResult.hpp"
#include "common/NetworkTimer.hpp"
#include "common/NetworkWorker.hpp"

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

    // old
    [[deprecated]] void type(NetworkRequestType newRequestType) &;

    [[deprecated]] void onReplyCreated(NetworkReplyCreatedCallback cb) &;
    [[deprecated]] void onError(NetworkErrorCallback cb) &;
    [[deprecated]] void onSuccess(NetworkSuccessCallback cb) &;

    [[deprecated]] void setPayload(const QByteArray &payload) &;
    [[deprecated]] void setUseQuickLoadCache(bool value) &;
    [[deprecated]] void setCaller(const QObject *caller) &;
    [[deprecated]] void setRawHeader(const char *headerName,
                                     const char *value) &;
    [[deprecated]] void setRawHeader(const char *headerName,
                                     const QByteArray &value) &;
    [[deprecated]] void setRawHeader(const char *headerName,
                                     const QString &value) &;
    [[deprecated]] void setTimeout(int ms) &;
    [[deprecated]] void setExecuteConcurrently(bool value) &;
    [[deprecated]] void makeAuthorizedV5(
        const QString &clientID, const QString &oauthToken = QString()) &;

    // new
    NetworkRequest type(NetworkRequestType newRequestType) &&;

    NetworkRequest onReplyCreated(NetworkReplyCreatedCallback cb) &&;
    NetworkRequest onError(NetworkErrorCallback cb) &&;
    NetworkRequest onSuccess(NetworkSuccessCallback cb) &&;

    NetworkRequest payload(const QByteArray &payload) &&;
    NetworkRequest cache() &&;
    NetworkRequest caller(const QObject *caller) &&;
    NetworkRequest header(const char *headerName, const char *value) &&;
    NetworkRequest header(const char *headerName, const QByteArray &value) &&;
    NetworkRequest header(const char *headerName, const QString &value) &&;
    NetworkRequest timeout(int ms) &&;
    NetworkRequest concurrent() &&;
    NetworkRequest authorizeTwitchV5(const QString &clientID,
                                     const QString &oauthToken = QString()) &&;

    void execute();

    static NetworkRequest twitchRequest(QUrl url);

private:
    void initializeDefaultValues();
};

}  // namespace chatterino
