#pragma once

#include "messages/search/MessagePredicate.hpp"

namespace chatterino {

/**
 * @brief MessagePredicate checking for the badges of a message.
 *
 * This predicate will only allow messages that are sent by a subscribed user 
 * who has a specified subtier (i.e. 1,2,3..) 
 */
class SubtierPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create an SubtierPredicate with a list of subtiers to search for.
     *
     * @param subtiers a list of subtiers that a message should contain 
     */
    SubtierPredicate(const QStringList &subtiers);

    /**
     * @brief Checks whether the message contains any of the subtiers passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message contains a subtier listed in the specified subtiers,
     *         false otherwise
     */
    bool appliesTo(const Message &message) override;

private:
    /// Holds the subtiers that will be searched for
    QStringList subtiers_;
};

}  // namespace chatterino
