#include "providers/twitch/TwitchBadge.hpp"

#include <QSet>

namespace chatterino {

// set of badge IDs that should be given specific flags.
// vanity flag is left out on purpose as it is our default flag
const QSet<QString> globalAuthority{"staff", "admin", "global_mod"};
const QSet<QString> predictions{"predictions"};
const QSet<QString> channelAuthority{"moderator", "vip", "broadcaster"};
const QSet<QString> subBadges{"subscriber", "founder"};

Badge::Badge(QString key, QString value)
    : key(std::move(key))
    , value(std::move(value))
{
    if (globalAuthority.contains(this->key))
    {
        this->flag = MessageElementFlag::BadgeGlobalAuthority;
    }
    else if (predictions.contains(this->key))
    {
        this->flag = MessageElementFlag::BadgePredictions;
    }
    else if (channelAuthority.contains(this->key))
    {
        this->flag = MessageElementFlag::BadgeChannelAuthority;
    }
    else if (subBadges.contains(this->key))
    {
        this->flag = MessageElementFlag::BadgeSubscription;
    }
}

bool Badge::operator==(const Badge &other) const
{
    return this->key == other.key && this->value == other.value;
}

}  // namespace chatterino
