#include "messages/search/BadgePredicate.hpp"

#include "util/Qt.hpp"

namespace chatterino {

BadgePredicate::BadgePredicate(const QStringList &badges)
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &entry : badges)
    {
        for (const auto &badge : entry.split(',', Qt::SkipEmptyParts))
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
}

bool BadgePredicate::appliesTo(const Message &message)
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
