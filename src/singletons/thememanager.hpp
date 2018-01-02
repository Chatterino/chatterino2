#pragma once

#include <QBrush>
#include <QColor>
#include <boost/signals2.hpp>
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
        return this->lightTheme;
    }

    struct TabColors {
        QColor text;
        struct Backgrounds {
            QBrush regular;
            QBrush hover;
            QBrush unfocused;
        } backgrounds;
    };

    struct Tabs {
        TabColors regular;
        TabColors selected;
        TabColors highlighted;
        TabColors newMessage;
    } tabs;

    struct Splits {
        QColor messageSeperator;
        QColor background;
        QColor border;
        QColor borderFocused;
        QColor dropPreview;

        struct Header {
            QColor border;
            QColor background;
            QColor text;
            // int margin;
        } header;

        struct Input {
            QColor border;
            QColor background;
            QColor selection;
            QColor text;
            QString styleSheet;
            // int margin;
        } input;
    } splits;

    struct Messages {
        struct TextColors {
            QColor regular;
            QColor caret;
            QColor link;
            QColor system;
        } textColors;

        struct Backgrounds {
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

    struct Scrollbars {
        QColor background;
        QColor thumb;
        QColor thumbSelected;
        // const int highlightsCount = 3;
        // QColor highlights[3];
    } scrollbars;

    struct Tooltip {
        QColor text;
        QColor background;
    } tooltip;

    void normalizeColor(QColor &color);

    void update();

    boost::signals2::signal<void()> updated;

private:
    pajlada::Settings::Setting<std::string> themeName;
    pajlada::Settings::Setting<double> themeHue;

    void actuallyUpdate(double hue, double multiplier);
    QColor blendColors(const QColor &color1, const QColor &color2, qreal ratio);

    double middleLookupTable[360] = {};
    double minLookupTable[360] = {};

    void fillLookupTableValues(double (&array)[360], double from, double to, double fromValue,
                               double toValue);

    bool lightTheme = false;

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets;

    friend class WindowManager;
};

}  // namespace chatterino
}
