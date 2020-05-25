#include "HighlightBadge.hpp"
#include "singletons/Resources.hpp"

namespace chatterino {

QColor HighlightBadge::FALLBACK_HIGHLIGHT_COLOR = QColor(127, 63, 73, 127);

bool HighlightBadge::operator==(const HighlightBadge &other) const
{
    return std::tie(this->badgeName_, this->badgeVersion_, this->displayName_,
                    this->hasSound_, this->hasAlert_, this->soundUrl_,
                    this->color_) ==
           std::tie(other.badgeName_, other.badgeVersion_, other.displayName_,
                    other.hasSound_, other.hasAlert_, other.soundUrl_,
                    other.color_);
}

HighlightBadge::HighlightBadge(const QString &badgeName,
                               const QString &badgeVersion,
                               const QString &displayName, bool hasAlert,
                               bool hasSound, const QString &soundUrl,
                               QColor color)
    : badgeName_(badgeName)
    , badgeVersion_(badgeVersion)
    , displayName_(displayName)
    , hasAlert_(hasAlert)
    , hasSound_(hasSound)
    , soundUrl_(soundUrl)
{
    this->color_ = std::make_shared<QColor>(color);
}

HighlightBadge::HighlightBadge(const QString &badgeName,
                               const QString &badgeVersion,
                               const QString &displayName, bool hasAlert,
                               bool hasSound, const QString &soundUrl,
                               std::shared_ptr<QColor> color)
    : badgeName_(badgeName)
    , badgeVersion_(badgeVersion)
    , displayName_(displayName)
    , hasAlert_(hasAlert)
    , hasSound_(hasSound)
    , soundUrl_(soundUrl)
    , color_(color)
{
}

const QString &HighlightBadge::badgeName() const
{
    return this->badgeName_;
}

const QString &HighlightBadge::badgeVersion() const
{
    return this->badgeVersion_;
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
    return this->badgeName_.compare(badge.key_, Qt::CaseInsensitive) == 0 &&
           this->badgeVersion_.compare(badge.value_, Qt::CaseInsensitive) == 0;
}

QString HighlightBadge::identifier() const
{
    return this->badgeName_ + "." + this->badgeVersion_;
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
