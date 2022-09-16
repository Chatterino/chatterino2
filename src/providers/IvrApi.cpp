#include "IvrApi.hpp"
#include <qurl.h>

#include "common/APIRequest.hpp"
#include "common/NetworkCommon.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <QUrlQuery>

namespace chatterino {

static IvrApi *instance = nullptr;

APIRequest<IvrSubage, APIRequestNoErrorValue> IvrApi::subage(
    const QString &userName, const QString &channelName)
{
    assert(!userName.isEmpty() && !channelName.isEmpty());
    auto url = QString("https://api.ivr.fi/twitch/subage/%1/%2")
                   .arg(userName)
                   .arg(channelName);
    return APIRequest<IvrSubage, APIRequestNoErrorValue>(
        QUrl(url), NetworkRequestType::Get,
        [](NetworkResult result) -> IvrSubage {
            return result.parseJson();
        },
        [](NetworkResult result) {
            qCWarning(chatterinoIvr)
                << "Failed IVR API Call!" << result.status()
                << QString(result.getData());
            return APIRequestNoErrorValue{};
        });
}

void IvrApi::getBulkEmoteSets(QString emoteSetList,
                              ResultCallback<QJsonArray> successCallback,
                              IvrFailureCallback failureCallback)
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("set_id", emoteSetList);

    this->makeRequest("v2/twitch/emotes/sets", urlQuery)
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJsonArray();

            successCallback(root);

            return Success;
        })
        .onError([failureCallback](auto result) {
            qCWarning(chatterinoIvr)
                << "Failed IVR API Call!" << result.status()
                << QString(result.getData());
            failureCallback();
        })
        .execute();
}

NetworkRequest IvrApi::makeRequest(QString url, QUrlQuery urlQuery)
{
    assert(!url.startsWith("/"));

    const QString baseUrl("https://api.ivr.fi/");
    QUrl fullUrl(baseUrl + url);
    fullUrl.setQuery(urlQuery);

    return NetworkRequest(fullUrl).timeout(5 * 1000).header("Accept",
                                                            "application/json");
}

void IvrApi::initialize()
{
    assert(instance == nullptr);

    instance = new IvrApi();
}

IvrApi *getIvr()
{
    assert(instance != nullptr);

    return instance;
}

}  // namespace chatterino
