#pragma once

#include "common/NetworkCommon.hpp"
#include "util/QObjectRef.hpp"

#include <QNetworkRequest>
#include <functional>

class QNetworkReply;

namespace chatterino {

class NetworkResult;

class NetworkRequester : public QObject
{
    Q_OBJECT

signals:
    void requestUrl();
};

class NetworkWorker : public QObject
{
    Q_OBJECT

signals:
    void doneUrl();
};

struct NetworkData {
    NetworkData();
    ~NetworkData();

    QNetworkRequest request_;
    bool hasCaller_{};
    QObjectRef<QObject> caller_;
    bool useQuickLoadCache_{};
    bool executeConcurrently{};

    NetworkReplyCreatedCallback onReplyCreated_;
    NetworkErrorCallback onError_;
    NetworkSuccessCallback onSuccess_;

    NetworkRequestType requestType_ = NetworkRequestType::Get;

    QByteArray payload_;

    // Timer that tracks the timeout
    // By default, there's no explicit timeout for the request
    // to enable the timer, the "setTimeout" function needs to be called before
    // execute is called
    bool hasTimeout_{};
    QTimer *timer_;

    QString getHash();

private:
    QString hash_;
};

void load(const std::shared_ptr<NetworkData> &data);

}  // namespace chatterino
