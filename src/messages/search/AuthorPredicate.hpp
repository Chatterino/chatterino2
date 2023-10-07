#pragma once

#include "messages/search/MessagePredicate.hpp"

#include <QString>
#include <QStringList>

namespace chatterino {

/**
 * @brief MessagePredicate checking for the author/sender of a message.
 *
 * This predicate will only allow messages that are sent by a list of users,
 * specified by their user names.
 */
class AuthorPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create an AuthorPredicate with a list of users to search for.
     *
     * @param authors one or more comma-separated user names that a message should be sent from
     * @param negate when set, excludes list of user names from results
     */
    AuthorPredicate(const QString &authors, bool negate);

protected:
    /**
     * @brief Checks whether the message is authored by any of the users passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message was authored by one of the specified users,
     *         false otherwise
     */
    bool appliesToImpl(const Message &message) override;

private:
    /// Holds the user names that will be searched for
    QStringList authors_;
};

}  // namespace chatterino
