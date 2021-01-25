#include "providers/twitch/api/Kraken.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchCommon.hpp"

namespace chatterino {

static Kraken *instance = nullptr;

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
    this->clientId = clientId;
    this->oauthToken = oauthToken;
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
