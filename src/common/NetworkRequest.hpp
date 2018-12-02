#pragma once

#include "common/NetworkCommon.hpp"
#include "common/NetworkRequester.hpp"
#include "common/NetworkResult.hpp"
#include "common/NetworkTimer.hpp"
#include "common/NetworkWorker.hpp"

namespace chatterino {

struct NetworkData;

class NetworkRequest final
{
    // Stores all data about the request that needs to be passed around to each
    // part of the request
    std::shared_ptr<NetworkData> data;

    // Timer that tracks the timeout
    // By default, there's no explicit timeout for the request
    // to enable the timer, the "setTimeout" function needs to be called before
    // execute is called
    std::shared_ptr<NetworkTimer> timer;

    // The NetworkRequest destructor will assert if executed_ hasn't been set to
    // true before dying
    bool executed_ = false;

public:
    explicit NetworkRequest(
        const std::string &url,
        NetworkRequestType requestType = NetworkRequestType::Get);
    explicit NetworkRequest(
        QUrl url, NetworkRequestType requestType = NetworkRequestType::Get);

    NetworkRequest(NetworkRequest &&other) = default;
    NetworkRequest &operator=(NetworkRequest &&other) = default;

    ~NetworkRequest();

    void setRequestType(NetworkRequestType newRequestType);

    void onReplyCreated(NetworkReplyCreatedCallback cb);
    void onError(NetworkErrorCallback cb);
    void onSuccess(NetworkSuccessCallback cb);

    void setPayload(const QByteArray &payload);
    void setUseQuickLoadCache(bool value);
    void setCaller(const QObject *caller);
    void setRawHeader(const char *headerName, const char *value);
    void setRawHeader(const char *headerName, const QByteArray &value);
    void setRawHeader(const char *headerName, const QString &value);
    void setTimeout(int ms);
    void setExecuteConcurrently(bool value);
    void makeAuthorizedV5(const QString &clientID,
                          const QString &oauthToken = QString());

    void execute();

    QString urlString() const;

private:
    // "invalid" data "invalid" is specified by the onSuccess callback
    Outcome tryLoadCachedFile();

    void doRequest();

public:
    // Helper creator functions
    static NetworkRequest twitchRequest(QUrl url);
};

}  // namespace chatterino
