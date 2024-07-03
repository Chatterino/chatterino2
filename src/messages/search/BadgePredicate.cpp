#include "messages/search/BadgePredicate.hpp"

#include "messages/Message.hpp"
#include "providers/twitch/TwitchBadge.hpp"

namespace chatterino {

BadgePredicate::BadgePredicate(const QString &badges, bool negate)
    : MessagePredicate(negate)
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &badge : badges.split(',', Qt::SkipEmptyParts))
    {
        // convert short form name of certain badges to formal name
        if (badge.compare("mod", Qt::CaseInsensitive) == 0)
        {
            this->badges_ << "moderator";
        }
        else if (badge.compare("sub", Qt::CaseInsensitive) == 0)
        {
            this->badges_ << "subscriber";
        }
        else if (badge.compare("prime", Qt::CaseInsensitive) == 0)
        {
            this->badges_ << "premium";
        }
        else
        {
            this->badges_ << badge;
        }
    }
}

bool BadgePredicate::appliesToImpl(const Message &message)
{
    for (const Badge &badge : message.badges)
    {
        if (badges_.contains(badge.key_, Qt::CaseInsensitive))
        {
            return true;
        }
    }

    return false;
}

}  // namespace chatterino
