#include "twitchuser.h"

namespace chatterino {
namespace twitch {
TwitchUser::TwitchUser(const QString &username, const QString &oauthToken,
                       const QString &oauthClient)
    : IrcUser2(username, username, username, "oauth:" + oauthToken)
{
    _oauthClient = oauthClient;
    _oauthToken = oauthToken;
}

const QString &TwitchUser::getOAuthClient() const
{
    return _oauthClient;
}

const QString &TwitchUser::getOAuthToken() const
{
    return _oauthToken;
}

bool TwitchUser::isAnon() const
{
    return IrcUser2::getNickName().startsWith("justinfan");
}
}
}
