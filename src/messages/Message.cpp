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
    DebugCount::increase("messages");
}

Message::~Message()
{
    DebugCount::decrease("messages");
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
    };

    QJsonArray badges;
    for (const auto &badge : this->badges)
    {
        badges.append(badge.key_);
    }
    msg["badges"_L1] = badges;

    QJsonObject badgeInfos;
    for (const auto &[key, value] : this->badgeInfos)
    {
        badgeInfos.insert(key, value);
    }
    msg["badgeInfos"_L1] = badgeInfos;

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

}  // namespace chatterino
