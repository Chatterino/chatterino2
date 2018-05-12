#pragma once

#include "controllers/accounts/account.hpp"
#include "providers/twitch/twitchuser.hpp"

#include <QColor>
#include <QString>

#include <set>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchAccount : public controllers::accounts::Account
{
public:
    TwitchAccount(const QString &username, const QString &oauthToken, const QString &oauthClient,
                  const QString &_userID);

    virtual QString toString() const override;

    const QString &getUserName() const;
    const QString &getOAuthToken() const;
    const QString &getOAuthClient() const;

    const QString &getUserId() const;

    // Attempts to update the users OAuth Client ID
    // Returns true if the value has changed, otherwise false
    bool setOAuthClient(const QString &newClientID);

    // Attempts to update the users OAuth Token
    // Returns true if the value has changed, otherwise false
    bool setOAuthToken(const QString &newOAuthToken);

    bool isAnon() const;

    void loadIgnores();
    void ignore(const QString &targetName, std::function<void(const QString &)> onFinished);
    void unignore(const QString &targetName, std::function<void(const QString &)> onFinished);

    std::set<TwitchUser> getIgnores() const;

    QColor color;

private:
    QString oauthClient;
    QString oauthToken;
    QString userName;
    QString userId;
    const bool _isAnon;

    mutable std::mutex ignoresMutex;
    std::set<TwitchUser> ignores;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
