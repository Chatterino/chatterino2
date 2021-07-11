#include "providers/twitch/api/Kraken.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchCommon.hpp"

namespace chatterino {

static Kraken *instance = nullptr;

void Kraken::getUserEmotes(TwitchAccount *account,
                           ResultCallback<KrakenEmoteSets> successCallback,
                           KrakenFailureCallback failureCallback)
{
    this->makeRequest(QString("users/%1/emotes").arg(account->getUserId()), {})
        .authorizeTwitchV5(account->getOAuthClient(), account->getOAuthToken())
        .onSuccess([successCallback](auto result) -> Outcome {
            auto data = result.parseJson();

            KrakenEmoteSets emoteSets(data);

            successCallback(emoteSets);

            return Success;
        })
        .onError([failureCallback](NetworkResult /*result*/) {
            // TODO: make better xd
            failureCallback();
        })
        .execute();
}

NetworkRequest Kraken::makeRequest(QString url, QUrlQuery urlQuery)
{
    assert(!url.startsWith("/"));

    if (this->clientId.isEmpty())
    {
        qCDebug(chatterinoTwitch)
            << "Kraken::makeRequest called without a client ID set BabyRage";
    }

    const QString baseUrl("https://api.twitch.tv/kraken/");

    QUrl fullUrl(baseUrl + url);

    fullUrl.setQuery(urlQuery);

    if (!this->oauthToken.isEmpty())
    {
        return NetworkRequest(fullUrl)
            .timeout(5 * 1000)
            .header("Accept", "application/vnd.twitchtv.v5+json")
            .header("Client-ID", this->clientId)
            .header("Authorization", "OAuth " + this->oauthToken);
    }

    return NetworkRequest(fullUrl)
        .timeout(5 * 1000)
        .header("Accept", "application/vnd.twitchtv.v5+json")
        .header("Client-ID", this->clientId);
}

void Kraken::update(QString clientId, QString oauthToken)
{
    this->clientId = std::move(clientId);
    this->oauthToken = std::move(oauthToken);
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
