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
            SBHighlight::Default,
            true,
        };
    }

    if (this->flags.has(MessageFlag::ElevatedMessage))
    {
        return {
            ColorProvider::instance().color(
                ColorType::ElevatedMessageHighlight),
            SBHighlight::Default,
            false,
            false,
            true,
        };
    }

    if (this->flags.has(MessageFlag::FirstMessage))
    {
        return {
            ColorProvider::instance().color(ColorType::FirstMessageHighlight),
            SBHighlight::Default,
            false,
            true,
        };
    }

    return {};
}

}  // namespace chatterino
