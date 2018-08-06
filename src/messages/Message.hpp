#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <QTime>
#include <cinttypes>
#include <memory>
#include <vector>

namespace chatterino {

struct Message : boost::noncopyable {
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

    Message();
    ~Message();

    FlagsEnum<MessageFlags> flags;
    QTime parseTime;
    QString id;
    QString searchText;
    QString loginName;
    QString displayName;
    QString localizedName;
    QString timeoutUser;
    uint32_t count = 1;
    std::vector<std::unique_ptr<MessageElement>> elements;

    ScrollbarHighlight getScrollBarHighlight() const;
};

using MessagePtr = std::shared_ptr<const Message>;

}  // namespace chatterino
