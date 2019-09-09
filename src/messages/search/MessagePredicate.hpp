#pragma once

#include "messages/Message.hpp"

#include <memory>

namespace chatterino {

/**
 * @brief Abstract base class for message predicates.
 *
 * Message predicates define certain features a message can satisfy.
 * Features are represented by classes derived from this abstract class.
 * A derived class must override `appliesTo` in order to test for the desired
 * feature.
 */
class MessagePredicate
{
public:
    /**
     * @brief Checks whether this predicate applies to the passed message.
     *
     * Implementations of `appliesTo` should never change the message's content
     * in order to be compatible with other MessagePredicates.
     *
     * @param message the message to check for this predicate
     * @return true if this predicate applies, false otherwise
     */
    virtual bool appliesTo(const MessagePtr message) = 0;
};
}  // namespace chatterino
