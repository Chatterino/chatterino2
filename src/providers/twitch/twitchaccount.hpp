#pragma once

#include <QColor>
#include <QString>

namespace chatterino {
namespace providers {
namespace twitch {
class TwitchAccount
{
public:
    TwitchAccount(const QString &username, const QString &oauthToken, const QString &oauthClient);

    const QString &getUserName() const;
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

    QColor color;

private:
    QString oauthClient;
    QString oauthToken;
    QString userId;
    QString userName;
    const bool _isAnon;
};
}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
