#include "messages/predicates/LinkPredicate.hpp"

namespace chatterino
{

LinkPredicate::LinkPredicate()
{
}

bool LinkPredicate::appliesTo(const MessagePtr message)
{
    // Borrowed and slightly abridged from https://stackoverflow.com/a/3809435
    static const QRegExp urlRegex(R"((https?:\/\/)?(www\.)?[-a-zA-Z0-9@:%._\+~#=]{2,256}\.[a-z]{2,4}\b([-a-zA-Z0-9@:%_\+.~#?&//=]*))", Qt::CaseInsensitive);

    if (!message)
        return false;

    return message->messageText.contains(urlRegex);
}

} // namespace chatterino
