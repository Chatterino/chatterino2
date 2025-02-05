#pragma once

#include "messages/MessageFlag.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "util/QStringHash.hpp"

#include <QColor>
#include <QTime>

#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <vector>

class QJsonObject;

namespace chatterino {
class MessageElement;
class MessageThread;
class Badge;
class ScrollbarHighlight;

struct Message;
using MessagePtr = std::shared_ptr<const Message>;
using MessagePtrMut = std::shared_ptr<Message>;
struct Message {
    Message();
    ~Message();

    Message(const Message &) = delete;
    Message &operator=(const Message &) = delete;

    Message(Message &&) = delete;
    Message &operator=(Message &&) = delete;

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
    // TODO: This field is used ambiguously, it could use a comment or a name change to
    // clarify the intent of the field
    QString loginName;
    QString displayName;
    QString localizedName;
    QString userID;
    QString timeoutUser;
    QString channelName;
    QColor usernameColor;
    QDateTime serverReceivedTime;
    std::vector<Badge> badges;
    std::unordered_map<QString, QString> badgeInfos;
    std::shared_ptr<QColor> highlightColor;
    // Each reply holds a reference to the thread. When every reply is dropped,
    // the reply thread will be cleaned up by the TwitchChannel.
    // The root of the thread does not have replyThread set.
    std::shared_ptr<MessageThread> replyThread;
    MessagePtr replyParent;
    uint32_t count = 1;
    std::vector<std::unique_ptr<MessageElement>> elements;

    ScrollbarHighlight getScrollBarHighlight() const;

    std::shared_ptr<ChannelPointReward> reward = nullptr;

    QJsonObject toJson() const;
};

}  // namespace chatterino
