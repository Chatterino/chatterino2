#pragma once

#include "messages/search/MessagePredicate.hpp"

namespace chatterino {

/**
 * @brief MessagePredicate checking whether a substring exists in the message.
 *
 * This predicate will only allow messages that contain a certain substring in
 * their `searchText`.
 */
class SubstringPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create a SubstringPredicate with a substring to search for.
     *
     * The passed string is searched for case-insensitively.
     *
     * @param search the string to search for in the message
     */
    SubstringPredicate(const QString &search);

    /**
     * @brief Checks whether the message contains the substring passed in the
     *        constructor.
     *
     * The check is done case-insensitively.
     *
     * @param message the message to check
     * @return true if the message contains the substring, false otherwise
     */
    bool appliesTo(const Message &message);

private:
    /// Holds the substring to search for in a message's `messageText`
    const QString search_;
};

}  // namespace chatterino
