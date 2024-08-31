#pragma once

#include "messages/search/MessagePredicate.hpp"

#include <QString>
#include <QStringList>

namespace chatterino {

/**
 * @brief MessagePredicate checking for the badges of a message.
 *
 * This predicate will only allow messages that are sent by a list of users,
 * specified by their user names, who have a badge specified (i.e 'staff').
 */
class BadgePredicate : public MessagePredicate
{
public:
    /**
     * @brief Create an BadgePredicate with a list of badges to search for.
     *
     * @param badges one or more comma-separated badges that a message should contain
     * @param negate when set, excludes list of badges from results
     */
    BadgePredicate(const QString &badges, bool negate);

protected:
    /**
     * @brief Checks whether the message contains any of the badges passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message contains a badge listed in the specified badges,
     *         false otherwise
     */
    bool appliesToImpl(const Message &message) override;

private:
    /// Holds the badges that will be searched for
    QStringList badges_;
};

}  // namespace chatterino
