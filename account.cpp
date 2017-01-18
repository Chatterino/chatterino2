#include "account.h"

namespace chatterino {

Account Account::anon("justinfan123", "", "");

Account::Account(QString username, QString oauthToken, QString oauthClient)
{
    this->oauthClient = oauthClient;
    this->oauthToken = oauthToken;
    this->username = username;
}
}
