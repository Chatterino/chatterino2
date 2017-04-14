#include "ircuser2.h"

namespace chatterino {

IrcUser2::IrcUser2(const QString &userName, const QString &nickName, const QString &realName,
                   const QString &password)
    : _userName(userName)
    , _nickName(nickName)
    , _realName(realName)
    , _password(password)
{
}

const QString &IrcUser2::getUserName() const
{
    return _userName;
}

const QString &IrcUser2::getNickName() const
{
    return _nickName;
}

const QString &IrcUser2::getRealName() const
{
    return _realName;
}

const QString &IrcUser2::getPassword() const
{
    return _password;
}
}
