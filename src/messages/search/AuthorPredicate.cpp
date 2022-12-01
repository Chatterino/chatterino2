#include "messages/search/AuthorPredicate.hpp"

#include "util/Qt.hpp"

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

bool AuthorPredicate::appliesTo(const Message &message)
{
    return this->isNegated ^
           (authors_.contains(message.displayName, Qt::CaseInsensitive) ||
            authors_.contains(message.loginName, Qt::CaseInsensitive));
}

}  // namespace chatterino
