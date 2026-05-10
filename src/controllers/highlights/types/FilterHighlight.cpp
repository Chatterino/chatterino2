// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/FilterHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"

#include <QIcon>
#include <QStringBuilder>

namespace chatterino::highlights {

FilterHighlight::FilterHighlight(QStringView _id)
    : id(_id)
{
    this->rebuildFilter();
}

bool FilterHighlight::isEnabled() const
{
    return this->enabled.value_or(true);
}

bool FilterHighlight::shouldShowInMentions() const
{
    return this->showInMentions.value_or(true);
}

void FilterHighlight::setShowInMentions(std::optional<bool> newValue)
{
    this->showInMentions = newValue;
}

bool FilterHighlight::shouldHighlightTaskbar() const
{
    return this->alert.value_or(true);
}

void FilterHighlight::setHighlightTaskbar(std::optional<bool> newValue)
{
    this->alert = newValue;
}

bool FilterHighlight::shouldPlaySound() const
{
    return this->playSound.value_or(false);
}

void FilterHighlight::setPlaySound(std::optional<bool> newValue)
{
    this->playSound = newValue;
}

QUrl FilterHighlight::getSoundUrl() const
{
    return this->customSoundURL;
}

void FilterHighlight::setSoundUrl(const QUrl &newValue)
{
    this->customSoundURL = newValue;
}

std::shared_ptr<QColor> FilterHighlight::getBackgroundColor() const
{
    return this->backgroundColor;
}

void FilterHighlight::setBackgroundColor(const QColor &newValue)
{
    this->backgroundColor = std::make_shared<QColor>(newValue);
}

QIcon FilterHighlight::getType() const
{
    return QIcon{":/settings/filters.svg"};
}

bool FilterHighlight::willPlayAnySound() const
{
    return this->playSound.value_or(false);
}

bool FilterHighlight::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->customSoundURL.isEmpty();
}

HighlightCheck FilterHighlight::buildCheck() const
{
    return {
        [highlight = *this](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage, const auto &flags, const auto self,
            const auto runContext) -> std::optional<HighlightResult> {
            (void)args;             // unused
            (void)badges;           // unused
            (void)senderName;       // unused
            (void)originalMessage;  // unused
            (void)flags;            // unused
            (void)self;             // unused

            assert(highlight.filter);

            auto res = highlight.filter->execute(runContext);
            if (!res.toBool())
            {
                return std::nullopt;
            }

            return HighlightResult{
                highlight.shouldHighlightTaskbar(), highlight.shouldPlaySound(),
                highlight.customSoundURL,           highlight.backgroundColor,
                highlight.shouldShowInMentions(),
            };
        },
    };
}

void FilterHighlight::rebuildFilter()
{
    auto res = filters::Filter::fromString(this->filterText);
    if (std::holds_alternative<filters::Filter>(res))
    {
        this->filter = std::make_shared<filters::Filter>(
            std::move(std::get<filters::Filter>(res)));
    }
    else
    {
        this->filter.reset();
        // TODO: Forward error
    }
}

QDebug operator<<(QDebug dbg, const FilterHighlight &v)
{
    const auto &backgroundColorPtr = v.getBackgroundColor();
    QColor backgroundColor;
    if (backgroundColorPtr)
    {
        backgroundColor = *backgroundColorPtr;
    }
    dbg.nospace() << "FilterHighlight("
                  << "name:" << v.name << ',' << "pattern:" << v.filterText
                  << ',' << "enabled:" << v.enabled << ','
                  << "showInMentions:" << v.shouldShowInMentions() << ','
                  << "alert:" << v.shouldHighlightTaskbar() << ','
                  << "playSound:" << v.playSound << ','
                  << "customSoundURL:" << v.getSoundUrl() << ','
                  << "backgroundColor:" << backgroundColor << ')';

    return dbg;
}

}  // namespace chatterino::highlights
