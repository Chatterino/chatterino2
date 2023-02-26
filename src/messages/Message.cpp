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
    else if (this->flags.has(MessageFlag::FirstMessage))
    {
        return SBHighlight(
            ColorProvider::instance().color(ColorType::FirstMessageHighlight),
            SBHighlight::Default, false, true);
    }

    return SBHighlight();
}

// Static
namespace {

}  // namespace

}  // namespace chatterino
