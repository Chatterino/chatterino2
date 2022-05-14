#include "messages/search/MessageFlagsPredicate.hpp"

#include "util/Qt.hpp"

namespace chatterino {

MessageFlagsPredicate::MessageFlagsPredicate(const QString &flags)
    : flags_()
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
    }
}

bool MessageFlagsPredicate::appliesTo(const Message &message)
{
    // Exclude timeout messages from system flag when timeout flag isn't present
    if (this->flags_.has(MessageFlag::System) &&
        !this->flags_.has(MessageFlag::Timeout))
        return message.flags.hasAny(flags_) &&
               !message.flags.has(MessageFlag::Timeout);
    return message.flags.hasAny(flags_);
}

}  // namespace chatterino
