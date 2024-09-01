#include "providers/IvrApi.hpp"

#include "common/network/NetworkResult.hpp"
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
        .onSuccess([successCallback, failureCallback](auto result) {
            auto root = result.parseJson();

            successCallback(root);
        })
        .onError([failureCallback](auto result) {
            qCWarning(chatterinoIvr)
                << "Failed IVR API Call!" << result.formatError()
                << QString(result.getData());
            failureCallback();
        })
        .execute();
}

NetworkRequest IvrApi::makeRequest(QString url, QUrlQuery urlQuery)
{
    assert(!url.startsWith("/"));

    const QString baseUrl("https://api.ivr.fi/v2/");
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
