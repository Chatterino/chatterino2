#include "controllers/highlights/HighlightBadge.hpp"

#include "providers/twitch/TwitchBadge.hpp"
#include "util/IrcHelpers.hpp"

namespace chatterino {

QColor HighlightBadge::FALLBACK_HIGHLIGHT_COLOR = QColor(127, 63, 73, 127);

bool HighlightBadge::operator==(const HighlightBadge &other) const
{
    return std::tie(this->badgeName_, this->displayName_, this->showInMentions_,
                    this->hasSound_, this->hasAlert_, this->soundUrl_,
                    this->color_) ==
           std::tie(other.badgeName_, other.displayName_, other.showInMentions_,
                    other.hasSound_, other.hasAlert_, other.soundUrl_,
                    other.color_);
}

HighlightBadge::HighlightBadge(const QString &badgeName,
                               const QString &displayName, bool showInMentions,
                               bool hasAlert, bool hasSound,
                               const QString &soundUrl, QColor color)
    : HighlightBadge(badgeName, displayName, showInMentions, hasAlert, hasSound,
                     soundUrl, std::make_shared<QColor>(color))
{
}

HighlightBadge::HighlightBadge(const QString &badgeName,
                               const QString &displayName, bool showInMentions,
                               bool hasAlert, bool hasSound,
                               const QString &soundUrl,
                               std::shared_ptr<QColor> color)
    : badgeName_(badgeName)
    , displayName_(displayName)
    , showInMentions_(showInMentions)
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

bool HighlightBadge::showInMentions() const
{
    return this->showInMentions_;
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
        auto parts = slashKeyValue(id);
        return parts.first.compare(badge.key_, Qt::CaseInsensitive) == 0 &&
               parts.second.compare(badge.value_, Qt::CaseInsensitive) == 0;
    }

    return id.compare(badge.key_, Qt::CaseInsensitive) == 0;
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
