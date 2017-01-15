#ifndef LINK_H
#define LINK_H

#include <QString>

class Link
{
public:
    enum Type {
        None,
        Url,
        CloseCurrentSplit,
        UserInfo,
        UserTimeout,
        UserBan,
        InsertText,
        ShowMessage,
    };

    Link();
    Link(Type type, const QString &value);

    bool
    isValid()
    {
        return m_type == None;
    }

    Type
    type()
    {
        return m_type;
    }

    const QString &
    value()
    {
        return m_value;
    }

private:
    Type m_type;
    QString m_value;
};

#endif  // LINK_H
