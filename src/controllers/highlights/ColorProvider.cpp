#include "controllers/highlights/ColorProvider.hpp"

#include "controllers/highlights/HighlightController.hpp"
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
    // Read settings for custom highlight colors and save them in map.
    // If no custom values can be found, set up default values instead.
    auto backgrounds = getApp()->themes->messages.backgrounds;

    QString customColor = getSettings()->selfHighlightColor;
    if (!customColor.isEmpty())
    {
        typeColorMap_.insert(
            {ColorType::SelfHighlight, std::make_shared<QColor>(customColor)});
    }
    else
    {
        typeColorMap_.insert(
            {ColorType::SelfHighlight,
             std::make_shared<QColor>(backgrounds.highlighted)});
    }

    customColor = getSettings()->subHighlightColor;
    if (!customColor.isEmpty())
    {
        typeColorMap_.insert(
            {ColorType::Subscription, std::make_shared<QColor>(customColor)});
    }
    else
    {
        typeColorMap_.insert(
            {ColorType::Subscription,
             std::make_shared<QColor>(backgrounds.subscription)});
    }

    customColor = getSettings()->whisperHighlightColor;
    if (!customColor.isEmpty())
    {
        typeColorMap_.insert(
            {ColorType::Whisper, std::make_shared<QColor>(customColor)});
    }
    else
    {
        typeColorMap_.insert(
            {ColorType::Whisper,
             std::make_shared<QColor>(backgrounds.highlighted)});
    }

    // Init default colors
    defaultColors_.emplace_back(31, 141, 43, 127);   // Green-ish
    defaultColors_.emplace_back(28, 126, 141, 127);  // Blue-ish
    defaultColors_.emplace_back(136, 141, 49, 127);  // Golden-ish
    defaultColors_.emplace_back(143, 48, 24, 127);   // Red-ish
    defaultColors_.emplace_back(28, 141, 117, 127);  // Cyan-ish
}

const std::shared_ptr<QColor> ColorProvider::color(ColorType type) const
{
    return typeColorMap_.at(type);
}

void ColorProvider::updateColor(ColorType type, QColor color)
{
    auto colorPtr = typeColorMap_.at(type);
    *colorPtr = color;
}

QSet<QColor> ColorProvider::recentColors() const
{
    QSet<QColor> retVal;
    for (auto phrase : getApp()->highlights->phrases)
    {
        // Ugly copying because HighlightPhrase::getColor returns a shared_ptr
        retVal.insert(QColor(phrase.getColor()->name(QColor::HexArgb)));
    }

    return retVal;
}

const std::vector<QColor> &ColorProvider::defaultColors() const
{
    return this->defaultColors_;
}

}  // namespace chatterino
