#pragma once

#include "common/Common.hpp"
#include "common/network/NetworkCommon.hpp"

#include <QHttpMultiPart>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>

#include <memory>
#include <optional>

class QNetworkReply;

namespace chatterino {

class NetworkResult;

class NetworkRequester : public QObject
{
    Q_OBJECT

Q_SIGNALS:
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

    NetworkSuccessCallback onSuccess;
    NetworkErrorCallback onError;
    NetworkFinallyCallback finally;

    NetworkRequestType requestType = NetworkRequestType::Get;

    QByteArray payload;
    std::unique_ptr<QHttpMultiPart, DeleteLater> multiPartPayload;

    /// By default, there's no explicit timeout for the request.
    /// To set a timeout, use NetworkRequest's timeout method
    std::optional<std::chrono::milliseconds> timeout{};
#ifndef NDEBUG
    bool ignoreSslErrors = false;  // for local eventsub
#endif

    QString getHash();

    void emitSuccess(NetworkResult &&result);
    void emitError(NetworkResult &&result);
    void emitFinally();

    QString typeString() const;

private:
    QString hash_;
};

void load(std::shared_ptr<NetworkData> &&data);

}  // namespace chatterino
