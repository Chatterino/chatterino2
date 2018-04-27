#pragma once

#include "messages/messageelement.hpp"
#include "singletons/helper/pubsubactions.hpp"
#include "util/flagsenum.hpp"
#include "widgets/helper/scrollbarhighlight.hpp"

#include <QTime>

#include <cinttypes>
#include <memory>
#include <vector>

#include "util/debugcount.hpp"

namespace chatterino {
namespace messages {

struct Message {
    Message()
    {
        util::DebugCount::increase("messages");
    }

    ~Message()
    {
        util::DebugCount::decrease("messages");
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
    };

    util::FlagsEnum<MessageFlags> flags;
    QTime parseTime;
    QString id;
    QString searchText;
    QString loginName;
    QString displayName;
    QString localizedName;
    QString timeoutUser;

    std::unique_ptr<singletons::BanAction> banAction;
    uint32_t count = 1;

    // Messages should not be added after the message is done initializing.
    void addElement(MessageElement *element);
    const std::vector<std::unique_ptr<MessageElement>> &getElements() const;

    // Scrollbar
    widgets::ScrollbarHighlight getScrollBarHighlight() const;

private:
    std::vector<std::unique_ptr<MessageElement>> elements;

public:
    static std::shared_ptr<Message> createSystemMessage(const QString &text);

    static std::shared_ptr<Message> createTimeoutMessage(const QString &username,
                                                         const QString &durationInSeconds,
                                                         const QString &reason, bool multipleTimes);

    static std::shared_ptr<Message> createTimeoutMessage(const singletons::BanAction &action,
                                                         uint32_t count = 1);
    static std::shared_ptr<Message> createUntimeoutMessage(const singletons::UnbanAction &action);
};

using MessagePtr = std::shared_ptr<Message>;

}  // namespace messages
}  // namespace chatterino
