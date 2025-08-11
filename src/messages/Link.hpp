#pragma once

#include <QString>

namespace chatterino {

struct Link {
public:
    enum Type {
        None,
        Url,
        UserInfo,
        UserWhisper,
        InsertText,
        UserAction,
        AutoModAllow,
        AutoModDeny,
        OpenAccountsPage,
        JumpToChannel,
        Reconnect,
        CopyToClipboard,
        ReplyToMessage,
        ViewThread,
        JumpToMessage,
    };

    Link();
    Link(Type getType, const QString &getValue);

    Type type;
    QString value;

    bool isValid() const;
    bool isUrl() const;
};

}  // namespace chatterino
