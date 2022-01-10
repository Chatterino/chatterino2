#pragma once

#include "messages/search/MessagePredicate.hpp"

namespace chatterino {

/**
 * @brief MessagePredicate checking for the channel a message was sent in.
 *
 * This predicate will only allow messages that are sent in a list of channels,
 * specified by their names.
 */
class ChannelPredicate : public MessagePredicate
{
public:
    /**
     * @brief Create a ChannelPredicate with a list of channels to search for.
     *
     * @param channels a list of channel names that a message should be sent in
     */
    ChannelPredicate(const QStringList &channels);

    /**
     * @brief Checks whether the message was sent in any of the channels passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message was sent in one of the specified channels,
     *         false otherwise
     */
    bool appliesTo(const Message &message);

private:
    /// Holds the channel names that will be searched for
    QStringList channels_;
};

}  // namespace chatterino
