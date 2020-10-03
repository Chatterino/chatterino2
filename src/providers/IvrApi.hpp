#pragma once

#include "common/NetworkRequest.hpp"
#include "messages/Link.hpp"

#include <functional>

namespace chatterino {

using IvrFailureCallback = std::function<void()>;
template <typename... T>
using ResultCallback = std::function<void(T...)>;

struct IvrSubage {
    const bool isSubHidden;
    const bool isSubbed;
    const QString subTier;
    const int totalSubMonths;
    const QString followingSince;

    IvrSubage(QJsonObject root)
        : isSubHidden(root.value("hidden").toBool())
        , isSubbed(root.value("subscribed").toBool())
        , subTier(root.value("meta").toObject().value("tier").toString())
        , totalSubMonths(
              root.value("cumulative").toObject().value("months").toInt())
        , followingSince(root.value("followedAt").toString())
    {
    }
};

class IvrApi final : boost::noncopyable
{
public:
    // https://api.ivr.fi/docs#tag/Twitch/paths/~1twitch~1subage~1{username}~1{channel}/get
    void getSubage(QString userName, QString channelName,
                   ResultCallback<IvrSubage> resultCallback,
                   IvrFailureCallback failureCallback);

    static void initialize();

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);
};

IvrApi *getIvr();

}  // namespace chatterino
