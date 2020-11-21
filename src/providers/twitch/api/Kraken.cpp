#include "providers/twitch/api/Kraken.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchCommon.hpp"

namespace chatterino {

static Kraken *instance = nullptr;

void Kraken::getChannel(QString userId,
                        ResultCallback<KrakenChannel> successCallback,
                        KrakenFailureCallback failureCallback)
{
    assert(!userId.isEmpty());

    this->makeRequest("channels/" + userId, {})
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();

            successCallback(root);

            return Success;
        })
        .onError([failureCallback](auto) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

void Kraken::getUser(QString userId, ResultCallback<KrakenUser> successCallback,
                     KrakenFailureCallback failureCallback)
{
    assert(!userId.isEmpty());

    this->makeRequest("users/" + userId, {})
        .onSuccess([successCallback, failureCallback](auto result) -> Outcome {
            auto root = result.parseJson();

            successCallback(root);

            return Success;
        })
        .onError([failureCallback](auto) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

NetworkRequest Kraken::makeRequest(QString url, QUrlQuery urlQuery)
{
    assert(!url.startsWith("/"));

    if (this->kclientId.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Kraken::makeRequest called without a client ID set BabyRage";
    }

    const QString baseUrl("https://api.twitch.tv/kraken/");

    QUrl fullUrl(baseUrl + url);

    fullUrl.setQuery(urlQuery);

    if (!this->koauthToken.isEmpty())
    {
        return NetworkRequest(fullUrl)
            .timeout(5 * 1000)
            .header("Accept", "application/vnd.twitchtv.v5+json")
            .header("Client-ID", this->kclientId)
            .header("Authorization", "OAuth " + this->koauthToken);
    }

    return NetworkRequest(fullUrl)
        .timeout(5 * 1000)
        .header("Accept", "application/vnd.twitchtv.v5+json")
        .header("Client-ID", this->kclientId);
}

void Kraken::update(QString clientId, QString oauthToken)
{
    this->kclientId = clientId;
    this->koauthToken = oauthToken;
}

void Kraken::initialize()
{
    assert(instance == nullptr);

    instance = new Kraken();

    getKraken()->update(getDefaultClientID(), "");
}

Kraken *getKraken()
{
    assert(instance != nullptr);

    return instance;
}

}  // namespace chatterino
