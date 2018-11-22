#pragma once

#include "common/Singleton.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <QBrush>
#include <QColor>
#include <pajlada/settings/setting.hpp>
#include <singletons/Settings.hpp>

namespace chatterino {

class WindowManager;

class Theme final : public Singleton
{
public:
    Theme();

    bool isLightTheme() const
    {
        return this->isLight_;
    }

    struct TabColors {
        QColor text;
        struct {
            QBrush regular;
            QBrush hover;
            QBrush unfocused;
        } backgrounds;
        struct {
            QColor regular;
            QColor hover;
            QColor unfocused;
        } line;
    };

    /// WINDOW
    struct {
        QColor background;
        QColor text;
        QColor borderUnfocused;
        QColor borderFocused;
    } window;

    /// TABS
    struct {
        TabColors regular;
        TabColors newMessage;
        TabColors highlighted;
        TabColors selected;
        QColor border;
        QColor bottomLine;
    } tabs;

    /// SPLITS
    struct {
        QColor messageSeperator;
        QColor background;
        QColor dropPreview;
        QColor dropPreviewBorder;
        QColor dropTargetRect;
        QColor dropTargetRectBorder;
        QColor resizeHandle;
        QColor resizeHandleBackground;

        struct {
            QColor border;
            QColor background;
            QColor text;
            QColor focusedText;
            // int margin;
        } header;

        struct {
            QColor border;
            QColor background;
            QColor selection;
            QColor focusedLine;
            QColor text;
            QString styleSheet;
            // int margin;
        } input;
    } splits;

    /// MESSAGES
    struct {
        struct {
            QColor regular;
            QColor caret;
            QColor link;
            QColor system;
        } textColors;

        struct {
            QColor regular;
            QColor alternate;
            QColor highlighted;
            QColor subscription;
            // QColor whisper;
        } backgrounds;

        QColor disabled;
        //        QColor seperator;
        //        QColor seperatorInner;
        QColor selection;
    } messages;

    /// SCROLLBAR
    struct {
        QColor background;
        QColor thumb;
        QColor thumbSelected;
        struct {
            QColor highlight;
            QColor subscription;
        } highlights;
    } scrollbars;

    /// TOOLTIP
    struct {
        QColor text;
        QColor background;
    } tooltip;

    void normalizeColor(QColor &color);

    void update();

    pajlada::Signals::NoArgSignal updated;

    QStringSetting themeName{"/appearance/theme/name", "Dark"};
    DoubleSetting themeHue{"/appearance/theme/hue", 0.0};

private:
    void actuallyUpdate(double hue, double multiplier);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal ratio);
    void fillLookupTableValues(double (&array)[360], double from, double to,
                               double fromValue, double toValue);

    double middleLookupTable_[360] = {};
    double minLookupTable_[360] = {};

    bool isLight_ = false;

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets_;

    friend class WindowManager;
};

}  // namespace chatterino
