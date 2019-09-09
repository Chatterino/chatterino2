#pragma once

#include "messages/search/MessagePredicate.hpp"

namespace chatterino
{

/**
 * @brief MessagePredicate checking whether a link exists in the message.
 *
 * This predicate will only allow messages that contain a link.
 */
class LinkPredicate : public MessagePredicate
{
public:
    LinkPredicate();

    /**
     * @brief Checks whether the message contains a link.
     *
     * @param message the message to check
     * @return true if the message contains a link, false otherwise
     */
    bool appliesTo(const Message &message);
};

} // namespace chatterino
