#ifndef AB_THEME_H
#define AB_THEME_H

#include <QBrush>
#include <QColor>
#include <common/ChatterinoSetting.hpp>

#ifdef AB_CUSTOM_THEME
#    define AB_THEME_CLASS BaseTheme
#else
#    define AB_THEME_CLASS Theme
#endif

namespace chatterino {

class Theme;

class AB_THEME_CLASS
{
public:
    bool isLightTheme() const;

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

    QColor accent{"#00aeef"};

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
        QColor dividerLine;
    } tabs;

    /// MESSAGES
    struct {
        struct {
            QColor regular;
            QColor caret;
            QColor link;
            QColor system;
            QColor chatPlaceholder;
        } textColors;

        struct {
            QColor regular;
            QColor alternate;
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

    void update();
    virtual void actuallyUpdate(double hue, double multiplier);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal ratio);

    pajlada::Signals::NoArgSignal updated;

    QStringSetting themeName{"/appearance/theme/name", "Dark"};
    DoubleSetting themeHue{"/appearance/theme/hue", 0.0};

private:
    bool isLight_ = false;
};

// Implemented in parent project if AB_CUSTOM_THEME is set.
// Otherwise implemented in BaseThemecpp
Theme *getTheme();

}  // namespace chatterino

#ifdef CHATTERINO
#    include "singletons/Theme.hpp"
#endif
#endif
