#include "RegexPredicate.hpp"

namespace chatterino {

RegexPredicate::RegexPredicate(const QString &regex)
    : regex_()
{
    this->regex_ =
        QRegularExpression(regex, QRegularExpression::CaseInsensitiveOption);
}

bool RegexPredicate::appliesTo(const Message &message)
{
    QRegularExpressionMatch match = regex_.match(message.searchText);

    return match.hasMatch();
}

}  // namespace chatterino