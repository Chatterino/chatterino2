#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/Message.hpp"
#include "messages/search/MessagePredicate.hpp"

namespace chatterino {

using MessageFlags = FlagsEnum<MessageFlag>;

/**
 * @brief MessagePredicate checking for message flags.
 *
 * This predicate will only allow messages with a list of flags.
 * Specified by user-friendly names for the flags.
 */
class MessageFlagsPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create a MessageFlagsPredicate with a list of flags to search for.
     *
     * The flags can be specified by user-friendly names.
     * "deleted" and "disabled" are used for the "Disabled" flag.
     * "sub" and "subscription" are used for the "Subscription" flag.
     * "timeout" is used for the "Timeout" flag.
     * "highlighted" is used for the "Highlighted" flag.
     * "system" is used for the "System" flag.
     *
     * @param flags a string comma seperated list of names for the flags a message should have
     * @param negate when set, excludes messages containg selected flags from results
     */
    MessageFlagsPredicate(const QString &flags, bool negate);

protected:
    /**
     * @brief Checks whether the message has any of the flags passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message has at least one of the specified flags,
     *         false otherwise
     */
    bool appliesToImpl(const Message &message) override;

private:
    /// Holds the flags that will be searched for
    MessageFlags flags_;
};

}  // namespace chatterino
