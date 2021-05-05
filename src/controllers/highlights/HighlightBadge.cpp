#include "HighlightBadge.hpp"

#include "singletons/Resources.hpp"

namespace chatterino {

QColor HighlightBadge::FALLBACK_HIGHLIGHT_COLOR = QColor(127, 63, 73, 127);

bool HighlightBadge::operator==(const HighlightBadge &other) const
{
    return std::tie(this->badgeName_, this->displayName_, this->hasSound_,
                    this->hasAlert_, this->soundUrl_, this->color_) ==
           std::tie(other.badgeName_, other.displayName_, other.hasSound_,
                    other.hasAlert_, other.soundUrl_, other.color_);
}

HighlightBadge::HighlightBadge(const QString &badgeName,
                               const QString &displayName, bool hasAlert,
                               bool hasSound, const QString &soundUrl,
                               QColor color)
    : HighlightBadge(badgeName, displayName, hasAlert, hasSound, soundUrl,
                     std::make_shared<QColor>(color))
{
}

HighlightBadge::HighlightBadge(const QString &badgeName,
                               const QString &displayName, bool hasAlert,
                               bool hasSound, const QString &soundUrl,
                               std::shared_ptr<QColor> color)
    : badgeName_(badgeName)
    , displayName_(displayName)
    , hasAlert_(hasAlert)
    , hasSound_(hasSound)
    , soundUrl_(soundUrl)
    , color_(color)
{
    // check badgeName at initialization to reduce cost per isMatch call
    this->hasVersions_ = badgeName.contains("/");
    this->isMulti_ = badgeName.contains(",");
    if (this->isMulti_)
    {
        this->badges_ = badgeName.split(",");
    }
}

const QString &HighlightBadge::badgeName() const
{
    return this->badgeName_;
}

const QString &HighlightBadge::displayName() const
{
    return this->displayName_;
}

bool HighlightBadge::hasAlert() const
{
    return this->hasAlert_;
}

bool HighlightBadge::hasSound() const
{
    return this->hasSound_;
}

bool HighlightBadge::isMatch(const Badge &badge) const
{
    if (this->isMulti_)
    {
        for (const auto &id : this->badges_)
        {
            if (this->compare(id, badge))
            {
                return true;
            }
        }
        return false;
    }
    else
    {
        return this->compare(this->badgeName_, badge);
    }
}

bool HighlightBadge::compare(const QString &id, const Badge &badge) const
{
    if (this->hasVersions_)
    {
        auto parts = id.split("/");
        if (parts.size() == 2)
        {
            return parts.at(0).compare(badge.key_, Qt::CaseInsensitive) == 0 &&
                   parts.at(1).compare(badge.value_, Qt::CaseInsensitive) == 0;
        }
        else
        {
            return parts.at(0).compare(badge.key_, Qt::CaseInsensitive) == 0;
        }
    }
    else
    {
        return id.compare(badge.key_, Qt::CaseInsensitive) == 0;
    }
}

bool HighlightBadge::hasCustomSound() const
{
    return !this->soundUrl_.isEmpty();
}

const QUrl &HighlightBadge::getSoundUrl() const
{
    return this->soundUrl_;
}

const std::shared_ptr<QColor> HighlightBadge::getColor() const
{
    return this->color_;
}

}  // namespace chatterino
