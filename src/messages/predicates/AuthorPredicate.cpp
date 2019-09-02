#include "messages/predicates/AuthorPredicate.hpp"

namespace chatterino
{

AuthorPredicate::AuthorPredicate(const QStringList& authors) : authors_(authors)
{
}

bool AuthorPredicate::appliesTo(const MessagePtr message)
{
    if (!message)
        return false;

    // TODO: Should this be a `contains` or an `equals`?
    return authors_.contains(message->displayName, Qt::CaseInsensitive);
}

} // namespace chatterino
