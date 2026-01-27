// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/MessageFlag.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "util/DebugCount.hpp"
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
class TwitchBadge;
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

    /// List of Twitch badges associated with this message
    std::vector<TwitchBadge> twitchBadges;

    /// Map of extra data associated with each Twitch badge
    std::unordered_map<QString, QString> twitchBadgeInfos;

    /// List of external badges associated with this message
    /// The badge should follow the following format: "provider:badgename". e.g.:
    ///  - betterttv:pro
    ///  - frankerfacez:mod
    ///  - 7tv:sub
    QStringList externalBadges;

    std::shared_ptr<QColor> highlightColor;
    // Each reply holds a reference to the thread. When every reply is dropped,
    // the reply thread will be cleaned up by the TwitchChannel.
    // The root of the thread does not have replyThread set.
    std::shared_ptr<MessageThread> replyThread;
    MessagePtr replyParent;
    enum class ReplyStatus : std::uint8_t {
        /// message has no reply thread, and message is not replyable
        ///
        /// e.g. due to message being deleted or too old
        NotReplyable,

        /// message has no reply thread, but message is replyable
        Replyable,

        /// message is part of a reply thread. both message & thread are replyable
        ReplyableWithThread,

        /// message is part of a reply thread. thread is replyable, but message is not replyable
        ///
        /// e.g. due to message being deleted or too old
        NotReplyableWithThread,

        /// message is part of a reply thread. neither reply or message is replyable
        ///
        /// e.g. due to message at the top of the thread being deleted
        NotReplyableDueToThread,
    };
    ReplyStatus isReplyable() const;
    uint32_t count = 1;

    /// Can this message be modified?
    ///
    /// Our rendering and layout code expects messages to be mostly immutable.
    /// Thus, when this flag is set, this message may not be modified.
    /// Only flags and this member can be modified safely (from the GUI thread).
    /// This is only used for plugins right now. This value is only ever set to
    /// true.
    mutable bool frozen = false;

    std::vector<std::unique_ptr<MessageElement>> elements;

    ScrollbarHighlight getScrollBarHighlight() const;

    std::shared_ptr<ChannelPointReward> reward = nullptr;

    QJsonObject toJson() const;

    void freeze() const
    {
        this->frozen = true;
    }
};

}  // namespace chatterino
