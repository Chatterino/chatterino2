#pragma once

#include <functional>

class QNetworkReply;

namespace chatterino {

class Outcome;
class NetworkResult;

using NetworkSuccessCallback = std::function<Outcome(NetworkResult)>;
using NetworkErrorCallback = std::function<void(NetworkResult)>;
using NetworkReplyCreatedCallback = std::function<void(QNetworkReply *)>;

enum class NetworkRequestType {
    Get,
    Post,
    Put,
    Delete,
};

}  // namespace chatterino
