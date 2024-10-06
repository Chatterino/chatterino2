#include "messages/layouts/MessageLayoutContext.hpp"

#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"

#include <algorithm>

namespace chatterino {

void MessageColors::applyTheme(Theme *theme, bool isOverlay,
                               int backgroundOpacity)
{
    auto applyColors = [this](const auto &src) {
        this->regularBg = src.backgrounds.regular;
        this->alternateBg = src.backgrounds.alternate;

        this->disabled = src.disabled;
        this->selection = src.selection;

        this->regularText = src.textColors.regular;
        this->linkText = src.textColors.link;
        this->systemText = src.textColors.system;
    };

    if (isOverlay)
    {
        this->channelBackground = theme->overlayMessages.background;
        this->channelBackground.setAlpha(std::clamp(backgroundOpacity, 0, 255));
        applyColors(theme->overlayMessages);
    }
    else
    {
        this->channelBackground = theme->splits.background;
        applyColors(theme->messages);
    }

    this->messageSeperator = theme->splits.messageSeperator;

    this->focusedLastMessageLine = theme->tabs.selected.backgrounds.regular;
    this->unfocusedLastMessageLine = theme->tabs.selected.backgrounds.unfocused;

    this->hasTransparency =
        this->regularBg.alpha() != 255 || this->alternateBg.alpha() != 255;
}

void MessagePreferences::connectSettings(Settings *settings,
                                         pajlada::Signals::SignalHolder &holder)
{
    settings->enableRedeemedHighlight.connect(
        [this](const auto &newValue) {
            this->enableRedeemedHighlight = newValue;
        },
        holder);

    settings->enableElevatedMessageHighlight.connect(
        [this](const auto &newValue) {
            this->enableElevatedMessageHighlight = newValue;
        },
        holder);

    settings->enableFirstMessageHighlight.connect(
        [this](const auto &newValue) {
            this->enableFirstMessageHighlight = newValue;
        },
        holder);

    settings->enableSubHighlight.connect(
        [this](const auto &newValue) {
            this->enableSubHighlight = newValue;
        },
        holder);

    settings->enableAutomodHighlight.connect(
        [this](const auto &newValue) {
            this->enableAutomodHighlight = newValue;
        },
        holder);

    settings->alternateMessages.connect(
        [this](const auto &newValue) {
            this->alternateMessages = newValue;
        },
        holder);

    settings->separateMessages.connect(
        [this](const auto &newValue) {
            this->separateMessages = newValue;
        },
        holder);

    settings->lastMessageColor.connect(
        [this](const auto &newValue) {
            if (newValue.isEmpty())
            {
                this->lastMessageColor = QColor();
            }
            else
            {
                this->lastMessageColor = QColor(newValue);
            }
        },
        holder);

    settings->lastMessagePattern.connect(
        [this](const auto &newValue) {
            this->lastMessagePattern = static_cast<Qt::BrushStyle>(newValue);
        },
        holder);
}

}  // namespace chatterino
