#include "providers/colors/ColorProvider.hpp"

#include "common/QLogging.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
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
    this->initTypeColorMap();
    this->initDefaultColors();
}

std::shared_ptr<QColor> ColorProvider::color(ColorType type) const
{
    return this->typeColorMap_.at(type);
}

QSet<QColor> ColorProvider::recentColors() const
{
    QSet<QColor> retVal;

    /*
     * Currently, only colors used in highlight phrases are considered. This
     * may change at any point in the future.
     */
    for (const auto &phrase : getSettings()->highlightedMessages)
    {
        retVal.insert(*phrase.getColor());
    }

    for (const auto &userHl : getSettings()->highlightedUsers)
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
    // Set up a signal to the respective setting for updating the color when it's changed
    auto initColor = [this](ColorType colorType, QStringSetting &setting,
                            QColor fallbackColor) {
        const auto &colorString = setting.getValue();
        QColor color(colorString);
        if (color.isValid())
        {
            this->typeColorMap_.insert({
                colorType,
                std::make_shared<QColor>(color),
            });
        }
        else
        {
            this->typeColorMap_.insert({
                colorType,
                std::make_shared<QColor>(fallbackColor),
            });
        }

        setting.connect(
            [this, colorType](const auto &colorString) {
                QColor color(colorString);
                if (color.isValid())
                {
                    // Update color based on the update from the setting
                    *this->typeColorMap_.at(colorType) = color;
                }
                else
                {
                    qCWarning(chatterinoCommon)
                        << "Updated"
                        << static_cast<std::underlying_type_t<ColorType>>(
                               colorType)
                        << "to invalid color" << colorString;
                }
            },
            false);
    };

    initColor(ColorType::SelfHighlight, getSettings()->selfHighlightColor,
              HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR);

    initColor(ColorType::SelfMessageHighlight,
              getSettings()->selfMessageHighlightColor,
              HighlightPhrase::FALLBACK_SELF_MESSAGE_HIGHLIGHT_COLOR);

    initColor(ColorType::Subscription, getSettings()->subHighlightColor,
              HighlightPhrase::FALLBACK_SUB_COLOR);

    initColor(ColorType::Whisper, getSettings()->whisperHighlightColor,
              HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR);

    initColor(ColorType::RedeemedHighlight,
              getSettings()->redeemedHighlightColor,
              HighlightPhrase::FALLBACK_REDEEMED_HIGHLIGHT_COLOR);

    initColor(ColorType::FirstMessageHighlight,
              getSettings()->firstMessageHighlightColor,
              HighlightPhrase::FALLBACK_FIRST_MESSAGE_HIGHLIGHT_COLOR);

    initColor(ColorType::ElevatedMessageHighlight,
              getSettings()->elevatedMessageHighlightColor,
              HighlightPhrase::FALLBACK_ELEVATED_MESSAGE_HIGHLIGHT_COLOR);

    initColor(ColorType::ThreadMessageHighlight,
              getSettings()->threadHighlightColor,
              HighlightPhrase::FALLBACK_THREAD_HIGHLIGHT_COLOR);

    initColor(ColorType::AutomodHighlight, getSettings()->automodHighlightColor,
              HighlightPhrase::FALLBACK_AUTOMOD_HIGHLIGHT_COLOR);
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
