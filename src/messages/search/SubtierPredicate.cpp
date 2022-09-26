#include "messages/search/SubtierPredicate.hpp"

#include "util/Qt.hpp"

namespace chatterino {

SubtierPredicate::SubtierPredicate(const QStringList &subtiers)
{
    // Check if any comma-seperated values were passed and transform those
    for (const auto &entry : subtiers)
    {
        for (const auto &subtier : entry.split(',', Qt::SkipEmptyParts))
        {
            this->subtiers_ << subtier;
        }
    }
}

bool SubtierPredicate::appliesTo(const Message &message)
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
