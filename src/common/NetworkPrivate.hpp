#pragma once

#include "common/NetworkCommon.hpp"

#include <QHttpMultiPart>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>

#include <functional>
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

struct NetworkData {
    NetworkData();
    ~NetworkData();

    QNetworkRequest request_;
    bool hasCaller_{};
    QPointer<QObject> caller_;
    bool cache_{};
    bool executeConcurrently_{};

    NetworkErrorCallback onError_;
    NetworkSuccessCallback onSuccess_;
    NetworkFinallyCallback finally_;

    NetworkRequestType requestType_ = NetworkRequestType::Get;

    QByteArray payload_;
    std::unique_ptr<QHttpMultiPart, DeleteLater> multiPartPayload_;

    // Timer that tracks the timeout
    // By default, there's no explicit timeout for the request
    // to enable the timer, the "setTimeout" function needs to be called before
    // execute is called
    bool hasTimeout_{};
    int timeoutMS_{};
    QObject *lifetimeManager_;

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
