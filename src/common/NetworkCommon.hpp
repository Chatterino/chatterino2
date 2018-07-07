#pragma once

#include <functional>

class QNetworkReply;

namespace chatterino {

class NetworkResult;

using NetworkSuccessCallback = std::function<bool(NetworkResult)>;
using NetworkErrorCallback = std::function<bool(int)>;
using NetworkReplyCreatedCallback = std::function<void(QNetworkReply *)>;

enum class NetworkRequestType {
    Get,
    Post,
    Put,
    Delete,
};

}  // namespace chatterino
