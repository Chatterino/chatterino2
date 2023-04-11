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

void MessagePreferences::applySettings(Settings *settings)
{
    this->enableRedeemedHighlight = settings->enableRedeemedHighlight;
    this->enableElevatedMessageHighlight =
        settings->enableElevatedMessageHighlight;
    this->enableFirstMessageHighlight = settings->enableFirstMessageHighlight;
    this->enableSubHighlight = settings->enableSubHighlight;

    this->alternateMessages = settings->alternateMessages;
    this->separateMessages = settings->separateMessages;

    this->lastMessageColor = settings->lastMessageColor.getValue().isEmpty()
                                 ? QColor()
                                 : QColor(settings->lastMessageColor);
    this->lastMessagePattern =
        static_cast<Qt::BrushStyle>(settings->lastMessagePattern);
}

void MessagePreferences::connectSettings(Settings *settings,
                                         pajlada::Signals::SignalHolder &holder)
{
    const auto apply = [this, settings]() {
        this->applySettings(settings);
    };
    const auto connect = [&](auto &setting) {
        setting.connect(apply, holder, false);
    };
    connect(settings->enableRedeemedHighlight);
    connect(settings->enableElevatedMessageHighlight);
    connect(settings->enableFirstMessageHighlight);
    connect(settings->enableSubHighlight);
    connect(settings->alternateMessages);
    connect(settings->separateMessages);
    connect(settings->lastMessageColor);
    connect(settings->lastMessagePattern);
}

}  // namespace chatterino
