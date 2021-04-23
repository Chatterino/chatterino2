#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/search/MessagePredicate.hpp"

namespace chatterino {

using MessageFlags = FlagsEnum<MessageFlag>;

/**
 * @brief MessagePredicate checking for message flags.
 *
 * This predicate will only allow messages with a list of flags.
 * specified by user friendly names for the flags.
 */
class MessageFlagsPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create a MessageFlagsPredicate with a list of flags to search for.
     *
     * @param flags a list of names for the flags a message should have
     */
    MessageFlagsPredicate(const QStringList &flags);

    /**
     * @brief Checks whether the message has any of the flags passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message has at least one of the specified flags,
     *         false otherwise
     */
    bool appliesTo(const Message &message);

private:
    /// Holds the flags that will be searched for
    MessageFlags flags_;
};

}  // namespace chatterino
