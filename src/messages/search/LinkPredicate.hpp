#pragma once

#include "messages/search/MessagePredicate.hpp"

#include <QString>

namespace chatterino {

/**
 * @brief MessagePredicate checking whether a link exists in the message.
 *
 * This predicate will only allow messages that contain a link.
 */
class LinkPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create an LinkPredicate
     * 
     * @param negate when set, excludes messages containing links from results
    */
    LinkPredicate(bool negate);

protected:
    /**
     * @brief Checks whether the message contains a link.
     *
     * @param message the message to check
     * @return true if the message contains a link, false otherwise
     */
    bool appliesToImpl(const Message &message) override;
};

}  // namespace chatterino
