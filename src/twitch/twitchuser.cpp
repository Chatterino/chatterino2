#include "twitchuser.hpp"

namespace chatterino {
namespace twitch {

TwitchUser::TwitchUser(const QString &username, const QString &oauthToken,
                       const QString &oauthClient)
    : IrcUser2(username, username, username, "oauth:" + oauthToken)
    , _oauthClient(oauthClient)
    , _oauthToken(oauthToken)
    , _isAnon(username.startsWith("justinfan"))
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

bool TwitchUser::isAnon() const
{
    return this->_isAnon;
}

}  // namespace twitch
}  // namespace chatterino
