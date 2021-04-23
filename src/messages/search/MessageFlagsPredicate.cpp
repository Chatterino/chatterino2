#include "messages/search/MessageFlagsPredicate.hpp"

namespace chatterino {

MessageFlagsPredicate::MessageFlagsPredicate(const QString &flags)
    : flags_()
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &flag : flags.split(',', QString::SkipEmptyParts))
    {
        if (flag == "deleted" || flag == "disabled")
        {
            this->flags_.set(MessageFlag::Disabled);
        }
        else if (flag == "sub" || flag == "subscription")
        {
            this->flags_.set(MessageFlag::Subscription);
        }
        else if (flag == "timeout")
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
    }
}

bool MessageFlagsPredicate::appliesTo(const Message &message)
{
    return message.flags.hasAny(flags_);
}

}  // namespace chatterino
