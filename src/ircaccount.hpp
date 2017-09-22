#pragma once

#include <QString>

namespace chatterino {

class IrcUser2
{
public:
    IrcUser2(const QString &userName, const QString &nickName, const QString &realName,
             const QString &password);

    const QString &getUserName() const;
    const QString &getNickName() const;
    const QString &getRealName() const;
    const QString &getPassword() const;

private:
    QString userName;
    QString nickName;
    QString realName;
    QString password;
};

}  // namespace chatterino
