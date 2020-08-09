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
             std::make_shared<QColor>(HighlightPhrase::FALLBACK_SUB_COLOR)});
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

    customColor = getSettings()->redeemedHighlightColor;
    if (QColor(customColor).isValid())
    {
        this->typeColorMap_.insert({ColorType::RedeemedHighlight,
                                    std::make_shared<QColor>(customColor)});
    }
    else
    {
        this->typeColorMap_.insert(
            {ColorType::RedeemedHighlight,
             std::make_shared<QColor>(
                 HighlightPhrase::FALLBACK_REDEEMED_HIGHLIGHT_COLOR)});
    }
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

    this->defaultColors_.push_back(HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR);
    this->defaultColors_.push_back(HighlightPhrase::FALLBACK_SUB_COLOR);
}

}  // namespace chatterino
