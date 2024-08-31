#include "util/DisplayBadge.hpp"

namespace chatterino {
DisplayBadge::DisplayBadge(QString displayName, QString badgeName)
    : displayName_(displayName)
    , badgeName_(badgeName)
{
}

QString DisplayBadge::displayName() const
{
    return this->displayName_;
}

QString DisplayBadge::badgeName() const
{
    return this->badgeName_;
}

}  // namespace chatterino
