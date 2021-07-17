#include "IvrApi.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <QUrlQuery>

namespace chatterino {

static IvrApi *instance = nullptr;

void IvrApi::getSubage(QString userName, QString channelName,
                       ResultCallback<IvrSubage> successCallback,
                       IvrFailureCallback failureCallback)
{
    assert(!userName.isEmpty() && !channelName.isEmpty());

    this->makeRequest(
            QString("twitch/subage/%1/%2").arg(userName).arg(channelName), {})
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();

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

void IvrApi::getBulkEmoteSets(QString emoteSetList,
                              ResultCallback<QJsonArray> successCallback,
                              IvrFailureCallback failureCallback,
                              std::function<void()> finallyCallback)
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
        .finally(std::move(finallyCallback))
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
