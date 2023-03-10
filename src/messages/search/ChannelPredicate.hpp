#pragma once

#include "messages/search/MessagePredicate.hpp"

#include <QString>
#include <QStringList>

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
     * @param channels one or more comma-separated channel names that a message should be sent in
     * @param negate when set, excludes list of channel names from results
     */
    ChannelPredicate(const QString &channels, bool negate);

protected:
    /**
     * @brief Checks whether the message was sent in any of the channels passed
     *        in the constructor.
     *
     * @param message the message to check
     * @return true if the message was sent in one of the specified channels,
     *         false otherwise
     */
    bool appliesToImpl(const Message &message) override;

private:
    /// Holds the channel names that will be searched for
    QStringList channels_;
};

}  // namespace chatterino
