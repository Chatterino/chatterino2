#pragma once

#include "messages/search/MessagePredicate.hpp"

#include <QRegularExpression>
#include <QString>

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
     * @param negate when set, excludes messages matching the regex from results
     */
    RegexPredicate(const QString &regex, bool negate);

protected:
    /**
     * @brief Checks whether the message matches the regex passed in the
     *        constructor
     *
     * The check is done case-insensitively.
     *
     * @param message the message to check
     * @return true if the message matches the regex, false otherwise
     */
    bool appliesToImpl(const Message &message) override;

private:
    /// Holds the regular expression to match the message against
    QRegularExpression regex_;
};

}  // namespace chatterino
