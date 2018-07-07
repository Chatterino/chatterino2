#pragma once

#include "common/NetworkCommon.hpp"

#include <QNetworkRequest>

#include <functional>

class QNetworkReply;

namespace chatterino {

class NetworkResult;

struct NetworkData {
    QNetworkRequest request_;
    const QObject *caller_ = nullptr;
    bool useQuickLoadCache_{};

    NetworkReplyCreatedCallback onReplyCreated_;
    NetworkErrorCallback onError_;
    NetworkSuccessCallback onSuccess_;

    NetworkRequestType requestType_ = NetworkRequestType::Get;

    QByteArray payload_;

    QString getHash();

    void writeToCache(const QByteArray &bytes);

private:
    QString hash_;
};

}  // namespace chatterino
