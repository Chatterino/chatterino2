#pragma once

#include "util/serialize-custom.hpp"

#include <QBrush>
#include <QColor>
#include <pajlada/settings/setting.hpp>

namespace chatterino {
namespace singletons {

class WindowManager;

class ThemeManager
{
    ThemeManager();

public:
    static ThemeManager &getInstance();

    inline bool isLightTheme() const
    {
        return this->isLight;
    }

    struct TabColors {
        QColor text;
        struct Backgrounds {
            QBrush regular;
            QBrush hover;
            QBrush unfocused;
        } backgrounds;
    };

    struct {
        TabColors regular;
        TabColors selected;
        TabColors highlighted;
        TabColors newMessage;
        QColor border;
    } tabs;

    struct {
        QColor messageSeperator;
        QColor background;
        QColor border;
        QColor borderFocused;
        QColor dropPreview;

        struct {
            QColor border;
            QColor background;
            QColor text;
            // int margin;
        } header;

        struct {
            QColor border;
            QColor background;
            QColor selection;
            QColor text;
            QString styleSheet;
            // int margin;
        } input;
    } splits;

    struct {
        struct {
            QColor regular;
            QColor caret;
            QColor link;
            QColor system;
        } textColors;

        struct {
            QColor regular;
            QColor highlighted;
            // QColor resub;
            // QColor whisper;
        } backgrounds;

        QColor disabled;
        //        QColor seperator;
        //        QColor seperatorInner;
        QColor selection;
    } messages;

    struct {
        QColor background;
        QColor thumb;
        QColor thumbSelected;
        // const int highlightsCount = 3;
        // QColor highlights[3];
    } scrollbars;

    struct {
        QColor text;
        QColor background;
    } tooltip;

    QColor windowBg;
    QColor windowText;

    void normalizeColor(QColor &color);

    void update();

    pajlada::Signals::NoArgSignal updated;

    pajlada::Settings::Setting<QString> themeName;
    pajlada::Settings::Setting<double> themeHue;

private:
    void actuallyUpdate(double hue, double multiplier);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal ratio);

    double middleLookupTable[360] = {};
    double minLookupTable[360] = {};

    void fillLookupTableValues(double (&array)[360], double from, double to, double fromValue,
                               double toValue);

    bool isLight = false;

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets;

    friend class WindowManager;
};

}  // namespace singletons
}  // namespace chatterino
