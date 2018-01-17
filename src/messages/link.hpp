#pragma once

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
        UserAction,
    };

    Link();
    Link(Type getType, const QString &getValue);

    bool isValid() const;
    Type getType() const;
    const QString &getValue() const;

private:
    Type type;
    QString value;
};

}  // namespace messages
}  // namespace chatterino
