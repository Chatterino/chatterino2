#include "messages/search/SubstringPredicate.hpp"

#include "messages/Message.hpp"

namespace chatterino {

SubstringPredicate::SubstringPredicate(const QString &search)
    : MessagePredicate(false)
    , search_(search)
{
}

bool SubstringPredicate::appliesToImpl(const Message &message)
{
    return message.searchText.contains(this->search_, Qt::CaseInsensitive);
}

}  // namespace chatterino
