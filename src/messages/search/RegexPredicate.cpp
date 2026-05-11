// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
    if (!this->regex_.isValid())
    {
        return false;
    }

    QRegularExpressionMatch match = this->regex_.match(message.messageText);

    return match.hasMatch();
}

}  // namespace chatterino
