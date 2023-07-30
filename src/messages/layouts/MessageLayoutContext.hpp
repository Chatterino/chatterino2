#pragma once

#include <QColor>
#include <QPainter>

namespace pajlada::Signals {
class SignalHolder;
}  // namespace pajlada::Signals

namespace chatterino {

class ColorProvider;
class Theme;
class Settings;
struct Selection;

struct MessageColors {
    QColor regular;
    QColor alternate;
    QColor disabled;
    QColor selection;
    QColor system;

    QColor messageSeperator;

    QColor focusedLastMessageLine;
    QColor unfocusedLastMessageLine;

    void applyTheme(Theme *theme);
};

struct MessagePreferences {
    QColor lastMessageColor;
    Qt::BrushStyle lastMessagePattern{};

    bool enableRedeemedHighlight{};
    bool enableElevatedMessageHighlight{};
    bool enableFirstMessageHighlight{};
    bool enableSubHighlight{};

    bool alternateMessages{};
    bool separateMessages{};

    void applySettings(Settings *settings);
    void connectSettings(Settings *settings,
                         pajlada::Signals::SignalHolder &holder);
};

struct MessagePaintContext {
    QPainter &painter;
    const Selection &selection;
    const ColorProvider &colorProvider;
    MessageColors messageColors;
    MessagePreferences preferences;

    int width{};
    int y{};
    int messageIndex{};
    bool isLastReadMessage{};
    bool isWindowFocused{};
    bool isMentions{};
};

}  // namespace chatterino
