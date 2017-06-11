#pragma once

#include "ircaccount.hpp"

#include <QString>

namespace chatterino {
namespace twitch {

class TwitchUser : public IrcUser2
{
public:
    TwitchUser(const QString &username, const QString &oauthToken, const QString &oauthClient);

    const QString &getOAuthToken() const;
    const QString &getOAuthClient() const;

    bool isAnon() const;

private:
    QString _oauthClient;
    QString _oauthToken;
};

}  // namespace twitch
}  // namespace chatterino
