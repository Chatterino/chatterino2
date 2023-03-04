#include "messages/Message.hpp"

#include "Application.hpp"
#include "MessageElement.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"
#include "util/IrcHelpers.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <algorithm>
#include <iterator>

using SBHighlight = chatterino::ScrollbarHighlight;

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

SBHighlight Message::getScrollBarHighlight() const
{
    if (this->flags.has(MessageFlag::Highlighted) ||
        this->flags.has(MessageFlag::HighlightedWhisper))
    {
        return SBHighlight(this->highlightColor);
    }
    else if (this->flags.has(MessageFlag::Subscription) &&
             getSettings()->enableSubHighlight)
    {
        return SBHighlight(
            ColorProvider::instance().color(ColorType::Subscription));
    }
    else if (this->flags.has(MessageFlag::RedeemedHighlight) ||
             this->flags.has(MessageFlag::RedeemedChannelPointReward))
    {
        return SBHighlight(
            ColorProvider::instance().color(ColorType::RedeemedHighlight),
            SBHighlight::Default, true);
    }
    else if (this->flags.has(MessageFlag::ElevatedMessage))
    {
        return SBHighlight(ColorProvider::instance().color(
                               ColorType::ElevatedMessageHighlight),
                           SBHighlight::Default, false, false, true);
    }
    else if (this->flags.has(MessageFlag::FirstMessage))
    {
        return SBHighlight(
            ColorProvider::instance().color(ColorType::FirstMessageHighlight),
            SBHighlight::Default, false, true);
    }

    return SBHighlight();
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

// Static
namespace {

}  // namespace

}  // namespace chatterino
