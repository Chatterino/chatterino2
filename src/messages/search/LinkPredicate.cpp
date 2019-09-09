#include "messages/search/LinkPredicate.hpp"
#include "common/LinkParser.hpp"

namespace chatterino {

LinkPredicate::LinkPredicate()
{
}

bool LinkPredicate::appliesTo(const MessagePtr message)
{
    if (!message)
        return false;

    for (const auto &word :
         message->messageText.split(' ', QString::SkipEmptyParts))
    {
        if (LinkParser(word).hasMatch())
            return true;
    }

    return false;
}

}  // namespace chatterino
