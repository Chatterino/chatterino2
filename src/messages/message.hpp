#pragma once

#include "messages/messageelement.hpp"
#include "widgets/helper/scrollbarhighlight.hpp"

#include <cinttypes>
#include <memory>
#include <vector>

#include <QTime>

namespace chatterino {
namespace messages {
class Message;

typedef std::shared_ptr<Message> MessagePtr;
typedef uint16_t MessageFlagsType;

class Message
{
public:
    enum MessageFlags : MessageFlagsType {
        None = 0,
        System = (1 << 0),
        Timeout = (1 << 1),
        Highlighted = (1 << 2),
        DoNotTriggerNotification = (1 << 3),  // disable notification sound
        Centered = (1 << 4),
        Disabled = (1 << 5),
        DisableCompactEmotes = (1 << 6),
        Collapsed = (1 << 7),
        DisconnectedMessage = (1 << 8),
    };

    // Elements
    // Messages should not be added after the message is done initializing.
    void addElement(MessageElement *element);
    const std::vector<std::unique_ptr<MessageElement>> &getElements() const;

    // Message flags
    MessageFlags getFlags() const;
    bool hasFlags(MessageFlags flags) const;
    //    void setFlags(MessageFlags flags);
    void addFlags(MessageFlags flags);
    void removeFlags(MessageFlags flags);

    // Parse Time
    const QTime &getParseTime() const;

    // Id
    const QString &getId() const;
    void setId(const QString &id);

    // Searching
    const QString &getSearchText() const;

    // Scrollbar
    widgets::ScrollbarHighlight getScrollBarHighlight() const;

    // Usernames
    QString loginName;
    QString displayName;
    QString localizedName;

    // Timeouts
    const QString &getTimeoutUser(const QString &value) const;
    void setTimeoutUser();

    // Static
    static MessagePtr createSystemMessage(const QString &text);

    static MessagePtr createTimeoutMessage(const QString &username,
                                           const QString &durationInSeconds, const QString &reason,
                                           bool multipleTimes);

private:
    static QRegularExpression *cheerRegex;

    MessageFlags flags = MessageFlags::None;
    QString timeoutUser;
    bool collapsedDefault = false;
    QTime parseTime;
    mutable QString searchText;
    QString id = "";

    std::vector<std::unique_ptr<MessageElement>> elements;
};

}  // namespace messages
}  // namespace chatterino
