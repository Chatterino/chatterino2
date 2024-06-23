#include "messages/search/SubtierPredicate.hpp"

#include "messages/Message.hpp"
#include "providers/twitch/TwitchBadge.hpp"

namespace chatterino {

SubtierPredicate::SubtierPredicate(const QString &subtiers, bool negate)
    : MessagePredicate(negate)
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &subtier : subtiers.split(',', Qt::SkipEmptyParts))
    {
        this->subtiers_ << subtier;
    }
}

bool SubtierPredicate::appliesToImpl(const Message &message)
{
    for (const Badge &badge : message.badges)
    {
        if (badge.key_ == "subscriber")
        {
            const auto &subTier =
                badge.value_.length() > 3 ? badge.value_.at(0) : '1';

            return subtiers_.contains(subTier);
        }
    }

    return false;
}

}  // namespace chatterino
