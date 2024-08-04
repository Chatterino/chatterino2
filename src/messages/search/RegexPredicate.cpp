#include "messages/search/RegexPredicate.hpp"

#include "messages/Message.hpp"

namespace chatterino {

RegexPredicate::RegexPredicate(const QString &regex, bool negate)
    : MessagePredicate(negate)
    , regex_(regex, QRegularExpression::CaseInsensitiveOption)
{
}

bool RegexPredicate::appliesToImpl(const Message &message)
{
    if (!regex_.isValid())
    {
        return false;
    }

    QRegularExpressionMatch match = regex_.match(message.messageText);

    return match.hasMatch();
}

}  // namespace chatterino
