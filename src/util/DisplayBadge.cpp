#include "DisplayBadge.hpp"

namespace chatterino {
DisplayBadge::DisplayBadge(QString displayName, QString badgeName,
                           QString badgeVersion)
    : displayName_(displayName)
    , badgeName_(badgeName)
    , badgeVersion_(badgeVersion)
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

QString DisplayBadge::badgeVersion() const
{
    return this->badgeVersion_;
}

QString DisplayBadge::identifier() const
{
    return this->badgeName_ + "." + this->badgeVersion_;
}

}  // namespace chatterino
