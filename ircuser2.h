#ifndef IRCUSER_H
#define IRCUSER_H

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
    QString _userName;
    QString _nickName;
    QString _realName;
    QString _password;
};
}

#endif  // IRCUSER_H
