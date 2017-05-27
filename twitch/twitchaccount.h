#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "ircaccount.h"

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

#endif  // ACCOUNT_H
