#include "messages/search/AuthorPredicate.hpp"

namespace chatterino {

AuthorPredicate::AuthorPredicate(const QStringList &authors)
    : authors_()
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &entry : authors)
    {
        for (const auto &author : entry.split(',', QString::SkipEmptyParts))
        {
            this->authors_ << author;
        }
    }
}

bool AuthorPredicate::appliesTo(const Message &message)
{
    return authors_.contains(message.displayName, Qt::CaseInsensitive) ||
           authors_.contains(message.loginName, Qt::CaseInsensitive);
}

}  // namespace chatterino
