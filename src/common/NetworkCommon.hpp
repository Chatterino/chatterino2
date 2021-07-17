#pragma once

#include <functional>
#include <vector>

#include <QString>

class QNetworkReply;

namespace chatterino {

class Outcome;
class NetworkResult;

using NetworkSuccessCallback = std::function<Outcome(NetworkResult)>;
using NetworkErrorCallback = std::function<void(NetworkResult)>;
using NetworkReplyCreatedCallback = std::function<void(QNetworkReply *)>;
using NetworkFinallyCallback = std::function<void()>;

enum class NetworkRequestType {
    Get,
    Post,
    Put,
    Delete,
    Patch,
};
const static std::vector<QString> networkRequestTypes{
    "GET",     //
    "POST",    //
    "PUT",     //
    "DELETE",  //
    "PATCH",   //
};

// parseHeaderList takes a list of headers in string form,
// where each header pair is separated by semicolons (;) and the header name and value is divided by a colon (:)
//
// We return a vector of pairs, where the first value is the header name and the second value is the header value
//
// e.g. "Authorization:secretkey;NextHeader:boo" will return [{"Authorization", "secretkey"}, {"NextHeader", "boo"}]
std::vector<std::pair<QByteArray, QByteArray>> parseHeaderList(
    const QString &headerListString);

}  // namespace chatterino
