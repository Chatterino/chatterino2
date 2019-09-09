#pragma once

#include "messages/search/MessagePredicate.hpp"

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
     * @param authors a list of user names that a message should be sent from
     */
    AuthorPredicate(const QStringList &authors);

    /**
     * @brief Checks whether the message is authored by any of the users passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message was authored by one of the specified users,
     *         false otherwise
     */
    bool appliesTo(const Message &message);

private:
    /// Holds the user names that will be searched for
    QStringList authors_;
};

}  // namespace chatterino
