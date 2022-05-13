#include "messages/search/LinkPredicate.hpp"

#include "common/LinkParser.hpp"
#include "util/Qt.hpp"

namespace chatterino {

LinkPredicate::LinkPredicate()
{
}

bool LinkPredicate::appliesTo(const Message &message)
{
    for (const auto &word : message.messageText.split(' ', Qt::SkipEmptyParts))
    {
        if (LinkParser(word).hasMatch())
            return true;
    }

    return false;
}

}  // namespace chatterino
