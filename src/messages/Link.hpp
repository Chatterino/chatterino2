#pragma once

#include <QString>
#include <common/Common.hpp>

namespace chatterino {

struct Link {
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

    Type type;
    QString value;

    bool isValid() const;
};

}  // namespace chatterino
