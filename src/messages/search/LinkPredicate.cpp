#include "messages/search/LinkPredicate.hpp"

#include "common/LinkParser.hpp"
#include "messages/Message.hpp"

namespace chatterino {

LinkPredicate::LinkPredicate(bool negate)
    : MessagePredicate(negate)
{
}

bool LinkPredicate::appliesToImpl(const Message &message)
{
    for (const auto &word : message.messageText.split(' ', Qt::SkipEmptyParts))
    {
        if (linkparser::parse(word).has_value())
        {
            return true;
        }
    }

    return false;
}

}  // namespace chatterino
