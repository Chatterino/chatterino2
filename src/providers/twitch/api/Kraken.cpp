#include "providers/twitch/api/Kraken.hpp"

#include "common/Outcome.hpp"
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
        .onError([failureCallback](auto result) {
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
        .onError([failureCallback](auto result) {
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
        qDebug()
            << "Helix::makeRequest called without a client ID set BabyRage";
        // return boost::none;
    }

    if (this->oauthToken.isEmpty())
    {
        qDebug()
            << "Helix::makeRequest called without an oauth token set BabyRage";
        // return boost::none;
    }

    const QString baseUrl("https://api.twitch.tv/kraken/");

    QUrl fullUrl(baseUrl + url);

    fullUrl.setQuery(urlQuery);

    return NetworkRequest(fullUrl)
        .timeout(5 * 1000)
        .header("Accept", "application/vnd.twitchtv.v5+json")
        .header("Client-ID", this->clientId)
        .header("Authorization", "OAuth " + this->oauthToken);
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
