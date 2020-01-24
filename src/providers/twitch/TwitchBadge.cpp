#include "providers/twitch/TwitchBadge.hpp"

#include <QSet>

namespace chatterino {

// set of badge IDs that should be given specific flags.
// vanity flag is left out on purpose as it is our default flag
const QSet<QString> globalAuthority{"staff", "admin", "global_mod"};
const QSet<QString> channelAuthority{"moderator", "vip", "broadcaster"};
const QSet<QString> subBadges{"subscriber", "founder"};

Badge::Badge(QString key, QString value)
    : key_(std::move(key))
    , value_(std::move(value))
{
    if (globalAuthority.contains(this->key_))
    {
        this->flag_ = MessageElementFlag::BadgeGlobalAuthority;
    }
    else if (channelAuthority.contains(this->key_))
    {
        this->flag_ = MessageElementFlag::BadgeChannelAuthority;
    }
    else if (subBadges.contains(this->key_))
    {
        this->flag_ = MessageElementFlag::BadgeSubscription;
    }
}

}  // namespace chatterino
