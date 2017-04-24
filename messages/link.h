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

    bool isValid() const
    {
        return type != None;
    }

    Type getType() const
    {
        return type;
    }

    const QString &getValue() const
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
