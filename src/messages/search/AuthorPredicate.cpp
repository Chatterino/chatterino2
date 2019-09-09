#include "messages/search/AuthorPredicate.hpp"

namespace chatterino {

AuthorPredicate::AuthorPredicate(const QStringList &authors)
    : authors_(authors)
{
}

bool AuthorPredicate::appliesTo(const MessagePtr message)
{
    if (!message)
        return false;

    return authors_.contains(message->displayName, Qt::CaseInsensitive);
}

}  // namespace chatterino
