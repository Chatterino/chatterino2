#include "messages/search/SubstringPredicate.hpp"

namespace chatterino {

SubstringPredicate::SubstringPredicate(const QString &search)
    : search_(search)
{
}

bool SubstringPredicate::appliesTo(const Message &message)
{
    return message.messageText.contains(this->search_, Qt::CaseInsensitive);
}

}  // namespace chatterino
