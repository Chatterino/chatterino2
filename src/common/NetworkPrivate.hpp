#pragma once

#include "common/Common.hpp"
#include "common/NetworkCommon.hpp"

#include <QHttpMultiPart>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>

#include <memory>

class QNetworkReply;

namespace chatterino {

class NetworkResult;

class NetworkRequester : public QObject
{
    Q_OBJECT

signals:
    void requestUrl();
};

class NetworkData
{
public:
    NetworkData();
    ~NetworkData();
    NetworkData(const NetworkData &) = delete;
    NetworkData(NetworkData &&) = delete;
    NetworkData &operator=(const NetworkData &) = delete;
    NetworkData &operator=(NetworkData &&) = delete;

    QNetworkRequest request;
    bool hasCaller{};
    QPointer<QObject> caller;
    bool cache{};
    bool executeConcurrently{};

    NetworkErrorCallback onError;
    NetworkSuccessCallback onSuccess;
    NetworkFinallyCallback finally;

    NetworkRequestType requestType = NetworkRequestType::Get;

    QByteArray payload;
    std::unique_ptr<QHttpMultiPart, DeleteLater> multiPartPayload;

    // Timer that tracks the timeout
    // By default, there's no explicit timeout for the request
    // to enable the timer, the "setTimeout" function needs to be called before
    // execute is called
    bool hasTimeout{};
    int timeoutMs{};

    QString getHash();

    void emitError(NetworkResult &&result);
    void emitSuccess(NetworkResult &&result);
    void emitFinally();

    QLatin1String typeString() const;

private:
    QString hash_;
};

void load(std::shared_ptr<NetworkData> &&data);

}  // namespace chatterino
