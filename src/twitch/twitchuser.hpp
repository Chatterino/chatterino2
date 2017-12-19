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
    const QString &getUserId() const;
    void setUserId(const QString &id);

    void setOAuthClient(const QString &newClientID);
    void setOAuthToken(const QString &newClientID);

    bool isAnon() const;

private:
    QString _oauthClient;
    QString _oauthToken;
    QString _userId;
    bool _isAnon;
};

}  // namespace twitch
}  // namespace chatterino
