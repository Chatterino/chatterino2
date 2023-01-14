#pragma once

#include <memory>

namespace chatterino {

struct Message;

/**
 * @brief Abstract base class for message predicates.
 *
 * Message predicates define certain features a message can satisfy.
 * Features are represented by classes derived from this abstract class.
 * A derived class must override `appliesToImpl` in order to test for the desired
 * feature.
 */
class MessagePredicate
{
public:
    virtual ~MessagePredicate() = default;

    /**
     * @brief Checks whether this predicate applies to the passed message
     *
     * Calls the derived classes `appliedTo` implementation, and respects the `isNegated_` flag
     * it's set.
     *
     * @param message the message to check for this predicate
     * @return true if this predicate applies, false otherwise
     **/
    bool appliesTo(const Message &message)
    {
        auto result = this->appliesToImpl(message);
        if (this->isNegated_)
        {
            return !result;
        }
        return result;
    }

protected:
    explicit MessagePredicate(bool negate)
        : isNegated_(negate)
    {
    }

    /**
     * @brief Checks whether this predicate applies to the passed message.
     *
     * Implementations of `appliesToImpl` should never change the message's content
     * in order to be compatible with other MessagePredicates.
     *
     * @param message the message to check for this predicate
     * @return true if this predicate applies, false otherwise
     */
    virtual bool appliesToImpl(const Message &message) = 0;

private:
    const bool isNegated_ = false;
};
}  // namespace chatterino
