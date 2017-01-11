#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "QString"

class Account
{
public:
    Account(QString username, QString oauthToken, QString oauthClient);

    static const Account *
    anon()
    {
        return m_anon;
    }

    const QString &
    username()
    {
        return m_username;
    }

    const QString &
    oauthToken()
    {
        return m_oauthToken;
    }

    const QString &
    oauthClient()
    {
        return m_oauthClient;
    }

    bool
    isAnon()
    {
        return m_username.startsWith("justinfan");
    }

private:
    const static Account *m_anon;

    QString m_username;
    QString m_oauthClient;
    QString m_oauthToken;
};

#endif  // ACCOUNT_H
