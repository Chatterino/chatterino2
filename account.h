#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QString>

class Account
{
public:
    Account(QString username, QString oauthToken, QString oauthClient);

    static const Account *
    getAnon()
    {
        return &anon;
    }

    const QString &
    getUsername() const
    {
        return username;
    }

    const QString &
    getOauthToken() const
    {
        return oauthToken;
    }

    const QString &
    getOauthClient() const
    {
        return oauthClient;
    }

    bool
    isAnon() const
    {
        return username.startsWith("justinfan");
    }

private:
    static Account anon;

    QString username;
    QString oauthClient;
    QString oauthToken;
};

#endif  // ACCOUNT_H
