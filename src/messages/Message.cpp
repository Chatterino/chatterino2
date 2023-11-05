#include "messages/Message.hpp"

#include "providers/colors/ColorProvider.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

namespace chatterino {

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

    return {};
}

std::shared_ptr<const Message> Message::cloneWith(
    const std::function<void(Message &)> &fn) const
{
    auto cloned = std::make_shared<Message>();
    cloned->flags = this->flags;
    cloned->parseTime = this->parseTime;
    cloned->id = this->id;
    cloned->searchText = this->searchText;
    cloned->messageText = this->messageText;
    cloned->loginName = this->loginName;
    cloned->displayName = this->displayName;
    cloned->localizedName = this->localizedName;
    cloned->timeoutUser = this->timeoutUser;
    cloned->channelName = this->channelName;
    cloned->usernameColor = this->usernameColor;
    cloned->serverReceivedTime = this->serverReceivedTime;
    cloned->badges = this->badges;
    cloned->badgeInfos = this->badgeInfos;
    cloned->highlightColor = this->highlightColor;
    cloned->replyThread = this->replyThread;
    cloned->count = this->count;
    std::transform(this->elements.cbegin(), this->elements.cend(),
                   std::back_inserter(cloned->elements),
                   [](const auto &element) {
                       return element->clone();
                   });
    fn(*cloned);
    return std::move(cloned);
}

}  // namespace chatterino
