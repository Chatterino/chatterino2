// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/Message.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "messages/MessageThread.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "util/QMagicEnum.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace chatterino {

using namespace literals;

Message::Message()
    : parseTime(QTime::currentTime())
{
    DebugCount::increase(DebugObject::Message);
}

Message::~Message()
{
    DebugCount::decrease(DebugObject::Message);
}

ScrollbarHighlight Message::getScrollBarHighlight() const
{
    if (this->flags.has(MessageFlag::Highlighted) ||
        this->flags.has(MessageFlag::HighlightedWhisper))
    {
        return {
            this->highlightColor,
        };
    }

    if (this->flags.has(MessageFlag::WatchStreak) &&
        getSettings()->enableWatchStreakHighlight)
    {
        return {
            ColorProvider::instance().color(ColorType::WatchStreak),
        };
    }

    if (this->flags.has(MessageFlag::Subscription) &&
        getSettings()->enableSubHighlight)
    {
        return {
            ColorProvider::instance().color(ColorType::Subscription),
        };
    }

    if (this->flags.has(MessageFlag::RedeemedHighlight) ||
        this->flags.has(MessageFlag::RedeemedChannelPointReward))
    {
        return {
            ColorProvider::instance().color(ColorType::RedeemedHighlight),
            ScrollbarHighlight::Default,
            true,
        };
    }

    if (this->flags.has(MessageFlag::ElevatedMessage))
    {
        return {
            ColorProvider::instance().color(
                ColorType::ElevatedMessageHighlight),
            ScrollbarHighlight::Default,
            false,
            false,
            true,
        };
    }

    if (this->flags.has(MessageFlag::FirstMessage))
    {
        return {
            ColorProvider::instance().color(ColorType::FirstMessageHighlight),
            ScrollbarHighlight::Default,
            false,
            true,
        };
    }

    if (this->flags.has(MessageFlag::AutoModOffendingMessage) ||
        this->flags.has(MessageFlag::AutoModOffendingMessageHeader))
    {
        return {
            ColorProvider::instance().color(ColorType::AutomodHighlight),
        };
    }

    return {};
}

QJsonObject Message::toJson() const
{
    QJsonObject msg{
        {"flags"_L1, qmagicenum::enumFlagsName(this->flags.value())},
        {"id"_L1, this->id},
        {"searchText"_L1, this->searchText},
        {"messageText"_L1, this->messageText},
        {"loginName"_L1, this->loginName},
        {"displayName"_L1, this->displayName},
        {"localizedName"_L1, this->localizedName},
        {"userID"_L1, this->userID},
        {"timeoutUser"_L1, this->timeoutUser},
        {"channelName"_L1, this->channelName},
        {"usernameColor"_L1, this->usernameColor.name(QColor::HexArgb)},
        {"count"_L1, static_cast<qint64>(this->count)},
        {"serverReceivedTime"_L1,
         this->serverReceivedTime.toString(Qt::ISODate)},
        {"frozen"_L1, this->frozen},
    };

    QJsonArray twitchBadges;
    for (const auto &badge : this->twitchBadges)
    {
        twitchBadges.append(badge.key_);
    }
    msg["twitchBadges"_L1] = twitchBadges;

    QJsonObject twitchBadgeInfos;
    for (const auto &[key, value] : this->twitchBadgeInfos)
    {
        twitchBadgeInfos.insert(key, value);
    }
    msg["twitchBadgeInfos"_L1] = twitchBadgeInfos;

    msg["externalBadges"_L1] = QJsonArray::fromStringList(this->externalBadges);

    if (this->highlightColor)
    {
        msg["highlightColor"_L1] = this->highlightColor->name(QColor::HexArgb);
    }

    if (this->replyThread)
    {
        msg["replyThread"_L1] = this->replyThread->toJson();
    }

    if (this->replyParent)
    {
        msg["replyParent"_L1] = this->replyParent->id;
    }

    if (this->reward)
    {
        msg["reward"_L1] = this->reward->toJson();
    }

    // XXX: figure out if we can add this in tests
    if (!getApp()->isTest())
    {
        msg["parseTime"_L1] = this->parseTime.toString(Qt::ISODate);
    }

    QJsonArray elements;
    for (const auto &element : this->elements)
    {
        elements.append(element->toJson());
    }
    msg["elements"_L1] = elements;

    return msg;
}

Message::ReplyStatus Message::isReplyable() const
{
    if (this->loginName.isEmpty())
    {
        // no replies can happen
        return ReplyStatus::NotReplyable;
    }

    constexpr int oneDayInSeconds = 24 * 60 * 60;
    bool messageReplyable = true;
    if (this->flags.hasAny({MessageFlag::System, MessageFlag::Subscription,
                            MessageFlag::Timeout, MessageFlag::Whisper,
                            MessageFlag::ModerationAction,
                            MessageFlag::InvalidReplyTarget}) ||
        this->serverReceivedTime.secsTo(QDateTime::currentDateTime()) >
            oneDayInSeconds)
    {
        messageReplyable = false;
    }

    if (this->replyThread != nullptr)
    {
        if (const auto &rootPtr = this->replyThread->root(); rootPtr != nullptr)
        {
            assert(this != rootPtr.get());
            if (rootPtr->isReplyable() == ReplyStatus::NotReplyable)
            {
                // thread parent must be replyable to be replyable
                return ReplyStatus::NotReplyableDueToThread;
            }

            return messageReplyable ? ReplyStatus::ReplyableWithThread
                                    : ReplyStatus::NotReplyableWithThread;
        }
    }

    return messageReplyable ? ReplyStatus::Replyable
                            : ReplyStatus::NotReplyable;
}

}  // namespace chatterino
