#include "providers/colors/ColorProvider.hpp"

#include "singletons/Theme.hpp"

namespace chatterino {

const ColorProvider &ColorProvider::instance()
{
    static ColorProvider instance;
    return instance;
}

ColorProvider::ColorProvider()
    : typeColorMap_()
    , defaultColors_()
{
    this->initTypeColorMap();
    this->initDefaultColors();
}

const std::shared_ptr<QColor> ColorProvider::color(ColorType type) const
{
    return this->typeColorMap_.at(type);
}

void ColorProvider::updateColor(ColorType type, QColor color)
{
    auto colorPtr = this->typeColorMap_.at(type);
    *colorPtr = color;
}

QSet<QColor> ColorProvider::recentColors() const
{
    QSet<QColor> retVal;

    /*
     * Currently, only colors used in highlight phrases are considered. This
     * may change at any point in the future.
     */
    for (auto phrase : getSettings()->highlightedMessages)
    {
        retVal.insert(*phrase.getColor());
    }

    for (auto userHl : getSettings()->highlightedUsers)
    {
        retVal.insert(*userHl.getColor());
    }

    // Insert preset highlight colors
    retVal.insert(*this->color(ColorType::SelfHighlight));
    retVal.insert(*this->color(ColorType::Subscription));
    retVal.insert(*this->color(ColorType::Whisper));

    return retVal;
}

const std::vector<QColor> &ColorProvider::defaultColors() const
{
    return this->defaultColors_;
}

void ColorProvider::initTypeColorMap()
{
    // Read settings for custom highlight colors and save them in map.
    // If no custom values can be found, set up default values instead.

    QString customColor = getSettings()->selfHighlightColor;
    if (QColor(customColor).isValid())
    {
        this->typeColorMap_.insert(
            {ColorType::SelfHighlight, std::make_shared<QColor>(customColor)});
    }
    else
    {
        this->typeColorMap_.insert(
            {ColorType::SelfHighlight,
             std::make_shared<QColor>(
                 HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR)});
    }

    customColor = getSettings()->subHighlightColor;
    if (QColor(customColor).isValid())
    {
        this->typeColorMap_.insert(
            {ColorType::Subscription, std::make_shared<QColor>(customColor)});
    }
    else
    {
        this->typeColorMap_.insert(
            {ColorType::Subscription,
             std::make_shared<QColor>(
                 HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR)});
    }

    customColor = getSettings()->whisperHighlightColor;
    if (QColor(customColor).isValid())
    {
        this->typeColorMap_.insert(
            {ColorType::Whisper, std::make_shared<QColor>(customColor)});
    }
    else
    {
        this->typeColorMap_.insert(
            {ColorType::Whisper,
             std::make_shared<QColor>(
                 HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR)});
    }
}

void ColorProvider::initDefaultColors()
{
    // Init default colors
    this->defaultColors_.emplace_back(31, 141, 43, 127);   // Green-ish
    this->defaultColors_.emplace_back(28, 126, 141, 127);  // Blue-ish
    this->defaultColors_.emplace_back(136, 141, 49, 127);  // Golden-ish
    this->defaultColors_.emplace_back(143, 48, 24, 127);   // Red-ish
    this->defaultColors_.emplace_back(28, 141, 117, 127);  // Cyan-ish

    auto backgrounds = getApp()->themes->messages.backgrounds;
    this->defaultColors_.push_back(HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR);
    this->defaultColors_.push_back(backgrounds.subscription);
}

}  // namespace chatterino
