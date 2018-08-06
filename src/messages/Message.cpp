#include "messages/Message.hpp"
#include "MessageElement.hpp"
#include "providers/twitch/PubsubActions.hpp"
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
    if (this->flags & Message::Highlighted) {
        return SBHighlight(SBHighlight::Highlight);
    } else if (this->flags & Message::Subscription) {
        return SBHighlight(SBHighlight::Subscription);
    }
    return SBHighlight();
}

// Static
namespace {

}  // namespace

}  // namespace chatterino
