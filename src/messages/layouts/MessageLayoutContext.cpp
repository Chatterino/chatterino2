#include "messages/layouts/MessageLayoutContext.hpp"

#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"

namespace chatterino {

void MessageColors::applyTheme(Theme *theme)
{
    this->regular = theme->messages.backgrounds.regular;
    this->alternate = theme->messages.backgrounds.alternate;

    this->disabled = theme->messages.disabled;
    this->selection = theme->messages.selection;
    this->system = theme->messages.textColors.system;

    this->messageSeperator = theme->splits.messageSeperator;

    this->focusedLastMessageLine = theme->tabs.selected.backgrounds.regular;
    this->unfocusedLastMessageLine = theme->tabs.selected.backgrounds.unfocused;
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
