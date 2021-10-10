#pragma once

#include "QRegularExpression"
#include "messages/search/MessagePredicate.hpp"

namespace chatterino {

/**
 * @brief MessagePredicate checking whether the message matches a given regex.
 *
 * This predicate will only allow messages whose `messageText` match the given
 * regex.
 */
class RegexPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create a RegexPredicate with a regex to match the message against.
     *
     * The message is being matched case-insensitively.
     *
     * @param regex the regex to match the message against
     */
    RegexPredicate(const QString &regex);

    /**
     * @brief Checks whether the message matches the regex passed in the
     *        constructor
     *
     * The check is done case-insensitively.
     *
     * @param message the message to check
     * @return true if the message matches the regex, false otherwise
     */
    bool appliesTo(const Message &message);

private:
    /// Holds the regular expression to match the message against
    QRegularExpression regex_;
};

}  // namespace chatterino