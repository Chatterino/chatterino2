#pragma once

#include "messages/MessageElement.hpp"

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

// TODO: Figure out if this could be a subset of Theme instead (e.g. Theme::MessageColors)
struct MessageColors {
    QColor channelBackground;

    // true if any of the background colors have transparency
    bool hasTransparency = false;

    QColor regularBg;
    QColor alternateBg;

    QColor disabled;
    QColor selection;

    QColor regularText;
    QColor linkText;
    QColor systemText;

    QColor messageSeperator;

    QColor focusedLastMessageLine;
    QColor unfocusedLastMessageLine;

    void applyTheme(Theme *theme, bool isOverlay, int backgroundOpacity);
};

// TODO: Explore if we can let settings own this
struct MessagePreferences {
    QColor lastMessageColor;
    Qt::BrushStyle lastMessagePattern{};

    bool enableRedeemedHighlight{};
    bool enableElevatedMessageHighlight{};
    bool enableFirstMessageHighlight{};
    bool enableSubHighlight{};
    bool enableAutomodHighlight{};

    bool alternateMessages{};
    bool separateMessages{};

    void connectSettings(Settings *settings,
                         pajlada::Signals::SignalHolder &holder);
};

struct MessagePaintContext {
    QPainter &painter;
    const Selection &selection;
    const ColorProvider &colorProvider;
    const MessageColors &messageColors;
    const MessagePreferences &preferences;

    // width of the area we have to draw on
    const int canvasWidth{};
    // whether the painting should be treated as if this view's window is focused
    const bool isWindowFocused{};
    // whether the painting should be treated as if this view is the special mentions view
    const bool isMentions{};

    // y coordinate we're currently painting at
    int y{};

    // Index of the message that is currently being painted
    // This index refers to the snapshot being used in the painting
    size_t messageIndex{};

    bool isLastReadMessage{};
};

struct MessageLayoutContext {
    const MessageColors &messageColors;
    MessageElementFlags flags;

    int width = 1;
    float scale = 1;
    float imageScale = 1;
};

}  // namespace chatterino
