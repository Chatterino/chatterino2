#pragma once

#include "common/FlagsEnum.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <QTime>
#include <boost/noncopyable.hpp>
#include <cinttypes>
#include <memory>
#include <vector>

namespace chatterino {
class MessageElement;

enum class MessageFlag : uint32_t {
    None = 0,
    System = (1 << 0),
    Timeout = (1 << 1),
    Highlighted = (1 << 2),
    DoNotTriggerNotification = (1 << 3),  // disable notification sound
    Centered = (1 << 4),
    Disabled = (1 << 5),
    DisableCompactEmotes = (1 << 6),
    Collapsed = (1 << 7),
    ConnectedMessage = (1 << 8),
    DisconnectedMessage = (1 << 9),
    Untimeout = (1 << 10),
    PubSub = (1 << 11),
    Subscription = (1 << 12),
    DoNotLog = (1 << 13),
    AutoMod = (1 << 14),
    RecentMessage = (1 << 15),
    Whisper = (1 << 16),
    HighlightedWhisper = (1 << 17),
    Debug = (1 << 18),
    Similar = (1 << 19),
};
using MessageFlags = FlagsEnum<MessageFlag>;

struct Message : boost::noncopyable {
    Message();
    ~Message();

    // Making this a mutable means that we can update a messages flags,
    // while still keeping Message constant. This means that a message's flag
    // can be updated without the renderer being made aware, which might be bad.
    // This is a temporary effort until we can figure out what the right
    // const-correct way to deal with this is.
    // This might bring race conditions with it
    mutable MessageFlags flags;
    QTime parseTime;
    QString id;
    QString searchText;
    QString messageText;
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
