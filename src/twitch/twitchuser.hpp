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

    // Attempts to update the users OAuth Client ID
    // Returns true if the value has changed, otherwise false
    bool setOAuthClient(const QString &newClientID);

    // Attempts to update the users OAuth Token
    // Returns true if the value has changed, otherwise false
    bool setOAuthToken(const QString &newOAuthToken);

    bool isAnon() const;

private:
    QString _oauthClient;
    QString _oauthToken;
    QString _userId;
    const bool _isAnon;
};

}  // namespace twitch
}  // namespace chatterino
