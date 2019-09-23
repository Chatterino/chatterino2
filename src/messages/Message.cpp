#include "messages/Message.hpp"

#include "Application.hpp"
#include "MessageElement.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"
#include "util/IrcHelpers.hpp"

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
        return SBHighlight::newHighlight(this->highlightColor);
    }
    else if (this->flags.has(MessageFlag::Subscription))
    {
        return SBHighlight::newSubscription();
    }
    return SBHighlight();
}

// Static
namespace {

}  // namespace

}  // namespace chatterino
