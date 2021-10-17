#include "RegexPredicate.hpp"

namespace chatterino {

RegexPredicate::RegexPredicate(const QString &regex)
    : regex_(regex, QRegularExpression::CaseInsensitiveOption)
{
}

bool RegexPredicate::appliesTo(const Message &message)
{
    if (!regex_.isValid())
    {
        return false;
    }

    QRegularExpressionMatch match = regex_.match(message.messageText);

    return match.hasMatch();
}

}  // namespace chatterino