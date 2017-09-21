#include "ircaccount.hpp"

namespace chatterino {

IrcUser2::IrcUser2(const QString &_userName, const QString &_nickName, const QString &_realName,
                   const QString &_password)
    : userName(_userName)
    , nickName(_nickName)
    , realName(_realName)
    , password(_password)
{
}

const QString &IrcUser2::getUserName() const
{
    return this->userName;
}

const QString &IrcUser2::getNickName() const
{
    return this->nickName;
}

const QString &IrcUser2::getRealName() const
{
    return this->realName;
}

const QString &IrcUser2::getPassword() const
{
    return this->password;
}

}  // namespace chatterino
