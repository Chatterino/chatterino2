// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/colors/ColorProvider.hpp"

#include "controllers/highlights/types/All.hpp"
#include "controllers/highlights/types/SubscriptionsHighlight.hpp"
#include "singletons/Settings.hpp"

#include <QSet>

namespace chatterino {

const ColorProvider &ColorProvider::instance()
{
    static ColorProvider instance;
    return instance;
}

ColorProvider::ColorProvider()
{
    this->initDefaultColors();
}

QSet<QColor> ColorProvider::recentColors() const
{
    QSet<QColor> colors;

    const auto highlights = getSettings()->sharedHighlights.readOnly();

    for (const auto &highlight : *highlights)
    {
        auto enabled = highlights::isEnabled(highlight);

        if (!enabled)
        {
            continue;
        }

        const auto color = highlights::getBackgroundColor(highlight);
        if (!color)
        {
            continue;
        }
        if (!color->isValid())
        {
            continue;
        }
        colors.insert(*color);
    }

    return colors;
}

const std::vector<QColor> &ColorProvider::defaultColors() const
{
    return this->defaultColors_;
}

void ColorProvider::initDefaultColors()
{
    // Init default colors
    this->defaultColors_.emplace_back(75, 127, 107, 100);  // Teal
    this->defaultColors_.emplace_back(105, 127, 63, 100);  // Olive
    this->defaultColors_.emplace_back(63, 83, 127, 100);   // Blue
    this->defaultColors_.emplace_back(72, 127, 63, 100);   // Green

    this->defaultColors_.emplace_back(31, 141, 43, 115);  // Green
    this->defaultColors_.emplace_back(28, 126, 141, 90);  // Blue
    this->defaultColors_.emplace_back(136, 141, 49, 90);  // Golden
    this->defaultColors_.emplace_back(143, 48, 24, 127);  // Red
    this->defaultColors_.emplace_back(28, 141, 117, 90);  // Cyan

    this->defaultColors_.push_back(
        highlights::MessageHighlight::BACKGROUND_COLOR_DEFAULT);
    this->defaultColors_.push_back(
        highlights::SubscriptionsHighlight::BACKGROUND_COLOR_DEFAULT);
}

}  // namespace chatterino
