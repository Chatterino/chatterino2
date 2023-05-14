#pragma once

#include "common/NetworkRequest.hpp"
#include "providers/twitch/TwitchEmotes.hpp"

#include <boost/noncopyable.hpp>
#include <QJsonArray>
#include <QJsonObject>

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

    IvrSubage(const QJsonObject &root)
        : isSubHidden(root.value("statusHidden").toBool())
        , isSubbed(!root.value("meta").isNull())
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
    const QString channelId;
    const QString tier;
    const QJsonArray emotes;

    IvrEmoteSet(const QJsonObject &root)
        : setId(root.value("setID").toString())
        , displayName(root.value("channelName").toString())
        , login(root.value("channelLogin").toString())
        , channelId(root.value("channelID").toString())
        , tier(root.value("tier").toString())
        , emotes(root.value("emoteList").toArray())

    {
    }
};

struct IvrEmote {
    const QString code;
    const QString id;
    const QString setId;
    const QString url;
    const QString emoteType;
    const QString imageType;

    explicit IvrEmote(const QJsonObject &root)
        : code(root.value("code").toString())
        , id(root.value("id").toString())
        , setId(root.value("setID").toString())
        , url(QString(TWITCH_EMOTE_TEMPLATE)
                  .replace("{id}", this->id)
                  .replace("{scale}", "3.0"))
        , emoteType(root.value("type").toString())
        , imageType(root.value("assetType").toString())
    {
    }
};

class IvrApi final : boost::noncopyable
{
public:
    // https://api.ivr.fi/v2/docs/static/index.html#/Twitch/get_twitch_subage__user___channel_
    void getSubage(QString userName, QString channelName,
                   ResultCallback<IvrSubage> resultCallback,
                   IvrFailureCallback failureCallback);

    // https://api.ivr.fi/v2/docs/static/index.html#/Twitch/get_twitch_emotes_sets
    void getBulkEmoteSets(QString emoteSetList,
                          ResultCallback<QJsonArray> successCallback,
                          IvrFailureCallback failureCallback);

    static void initialize();

private:
    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery);
};

IvrApi *getIvr();

}  // namespace chatterino
