#include "account.h"

const Account *Account::m_anon = new Account("justinfan123", "", "");

Account::Account(QString username, QString oauthToken, QString oauthClient)
{
    m_oauthClient = oauthClient;
    m_oauthToken = oauthToken;
    m_username = username;
}
