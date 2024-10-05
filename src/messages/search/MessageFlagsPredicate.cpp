#include "messages/search/MessageFlagsPredicate.hpp"

namespace chatterino {

MessageFlagsPredicate::MessageFlagsPredicate(const QString &flags, bool negate)
    : MessagePredicate(negate)
    , flags_()
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &flag : flags.split(',', Qt::SkipEmptyParts))
    {
        if (flag == "deleted" || flag == "disabled")
        {
            this->flags_.set(MessageFlag::Disabled);
        }
        else if (flag == "sub" || flag == "subscription")
        {
            this->flags_.set(MessageFlag::Subscription);
        }
        else if (flag == "timeout" || flag == "ban")
        {
            this->flags_.set(MessageFlag::Timeout);
        }
        else if (flag == "highlighted")
        {
            this->flags_.set(MessageFlag::Highlighted);
        }
        else if (flag == "system")
        {
            this->flags_.set(MessageFlag::System);
        }
        else if (flag == "first-msg")
        {
            this->flags_.set(MessageFlag::FirstMessage);
        }
        else if (flag == "elevated-msg" || flag == "hype-chat")
        {
            this->flags_.set(MessageFlag::ElevatedMessage);
        }
        else if (flag == "cheer-msg")
        {
            this->flags_.set(MessageFlag::CheerMessage);
        }
        else if (flag == "redemption")
        {
            this->flags_.set(MessageFlag::RedeemedChannelPointReward);
            this->flags_.set(MessageFlag::RedeemedHighlight);
        }
        else if (flag == "reply")
        {
            this->flags_.set(MessageFlag::ReplyMessage);
        }
        else if (flag == "restricted")
        {
            this->flags_.set(MessageFlag::RestrictedMessage);
        }
        else if (flag == "monitored")
        {
            this->flags_.set(MessageFlag::MonitoredMessage);
        }
        else if (flag == "shared")
        {
            this->flags_.set(MessageFlag::SharedMessage);
        }
    }
}

bool MessageFlagsPredicate::appliesToImpl(const Message &message)
{
    // Exclude timeout messages from system flag when timeout flag isn't present
    if (this->flags_.has(MessageFlag::System) &&
        !this->flags_.has(MessageFlag::Timeout))
    {
        return message.flags.hasAny(flags_) &&
               !message.flags.has(MessageFlag::Timeout);
    }
    return message.flags.hasAny(flags_);
}

}  // namespace chatterino
