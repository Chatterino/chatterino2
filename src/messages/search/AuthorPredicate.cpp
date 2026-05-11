// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/search/AuthorPredicate.hpp"

#include "messages/Message.hpp"

namespace chatterino {

AuthorPredicate::AuthorPredicate(const QString &authors, bool negate)
    : MessagePredicate(negate)
    , authors_()
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &author : authors.split(',', Qt::SkipEmptyParts))
    {
        this->authors_ << author;
    }
}

bool AuthorPredicate::appliesToImpl(const Message &message)
{
    return this->authors_.contains(message.displayName, Qt::CaseInsensitive) ||
           this->authors_.contains(message.loginName, Qt::CaseInsensitive);
}

}  // namespace chatterino
