#pragma once

#include <functional>

#include "Common.hpp"

class QNetworkReply;

namespace chatterino {

class NetworkResult;

using NetworkSuccessCallback = std::function<Outcome(NetworkResult)>;
using NetworkErrorCallback = std::function<bool(int)>;
using NetworkReplyCreatedCallback = std::function<void(QNetworkReply *)>;

enum class NetworkRequestType {
    Get,
    Post,
    Put,
    Delete,
};

}  // namespace chatterino
