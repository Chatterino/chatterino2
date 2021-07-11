#pragma once

#include "common/NetworkRequest.hpp"
#include "messages/Link.hpp"

#include <boost/noncopyable.hpp>

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

struct IvrEmoteSet {
    const QString setId;
    const QString displayName;
    const QString login;
    const QString id;
    const QString tier;
    const QJsonArray emotes;

    IvrEmoteSet(QJsonObject root)
        : setId(root.value("setID").toString())
        , displayName(root.value("channelName").toString())
        , login(root.value("channelLogin").toString())
        , id(root.value("channelID").toString())
        , tier(root.value("tier").toString())
        , emotes(root.value("emotes").toArray())

    {
    }
};

struct IvrEmote {
    const QString code;
    const QString id;
    const QString setId;
    const QString url;

    IvrEmote(QJsonObject root)
        : code(root.value("token").toString())
        , id(root.value("id").toString())
        , setId(root.value("setID").toString())
        , url(root.value("url_3x").toString())
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

    // https://api.ivr.fi/docs#tag/Twitch/paths/~1twitch~1emoteset/get
    void getBulkEmoteSets(QString emoteSetList,
                          ResultCallback<QJsonArray> successCallback,
                          IvrFailureCallback failureCallback);

    static void initialize();

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);
};

IvrApi *getIvr();

}  // namespace chatterino
