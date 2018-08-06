#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <QTime>

#include <cinttypes>
#include <memory>
#include <vector>

#include "util/DebugCount.hpp"

namespace chatterino {

struct Message {
    Message()
        : parseTime(QTime::currentTime())
    {
        DebugCount::increase("messages");
    }

    ~Message()
    {
        DebugCount::decrease("messages");
    }

    enum MessageFlags : uint16_t {
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
        Untimeout = (1 << 9),
        PubSub = (1 << 10),
        Subscription = (1 << 11),
    };

    FlagsEnum<MessageFlags> flags;
    QTime parseTime;
    QString id;
    QString searchText;
    QString loginName;
    QString displayName;
    QString localizedName;
    QString timeoutUser;

    uint32_t count = 1;

    // Messages should not be added after the message is done initializing.
    void addElement(MessageElement *element);
    const std::vector<std::unique_ptr<MessageElement>> &getElements() const;

    // Scrollbar
    ScrollbarHighlight getScrollBarHighlight() const;

private:
    std::vector<std::unique_ptr<MessageElement>> elements_;

public:
    static std::shared_ptr<Message> createSystemMessage(const QString &text);
    static std::shared_ptr<Message> createMessage(const QString &text);

    static std::shared_ptr<Message> createTimeoutMessage(
        const QString &username, const QString &durationInSeconds,
        const QString &reason, bool multipleTimes);

    static std::shared_ptr<Message> createTimeoutMessage(
        const BanAction &action, uint32_t count = 1);
    static std::shared_ptr<Message> createUntimeoutMessage(
        const UnbanAction &action);
};

using MessagePtr = std::shared_ptr<Message>;

}  // namespace chatterino
