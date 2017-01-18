#ifndef LINK_H
#define LINK_H

#include <QString>

namespace chatterino {
namespace messages {

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
    Link(Type getType, const QString &getValue);

    bool
    getIsValid()
    {
        return type == None;
    }

    Type
    getType()
    {
        return type;
    }

    const QString &
    getValue()
    {
        return value;
    }

private:
    Type type;
    QString value;
};
}
}

#endif  // LINK_H
