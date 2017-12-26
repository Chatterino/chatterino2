#include "twitchuser.hpp"
#include "const.hpp"
#include "util/urlfetch.hpp"

namespace chatterino {
namespace twitch {

TwitchUser::TwitchUser(const QString &username, const QString &oauthToken,
                       const QString &oauthClient)
    : IrcUser2(username, username, username, "oauth:" + oauthToken)
    , _oauthClient(oauthClient)
    , _oauthToken(oauthToken)
    , _isAnon(username == ANONYMOUS_USERNAME)
{
}

const QString &TwitchUser::getOAuthClient() const
{
    return this->_oauthClient;
}

const QString &TwitchUser::getOAuthToken() const
{
    return this->_oauthToken;
}

const QString &TwitchUser::getUserId() const
{
    return this->_userId;
}

void TwitchUser::setUserId(const QString &id)
{
    this->_userId = id;
}

bool TwitchUser::setOAuthClient(const QString &newClientID)
{
    if (this->_oauthClient.compare(newClientID) == 0) {
        return false;
    }

    this->_oauthClient = newClientID;

    return true;
}

bool TwitchUser::setOAuthToken(const QString &newOAuthToken)
{
    if (this->_oauthToken.compare(newOAuthToken) == 0) {
        return false;
    }

    this->_oauthToken = newOAuthToken;

    return true;
}

bool TwitchUser::isAnon() const
{
    return this->_isAnon;
}

}  // namespace twitch
}  // namespace chatterino
