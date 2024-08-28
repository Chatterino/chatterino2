#pragma once

#include "common/ChatterinoSetting.hpp"
#include "singletons/Paths.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/settings/setting.hpp>
#include <QColor>
#include <QJsonObject>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QVariant>

#include <memory>
#include <optional>
#include <vector>

namespace chatterino {

class WindowManager;

struct ThemeDescriptor {
    QString key;

    // Path to the theme on disk
    // Can be a Qt resource path
    QString path;

    // Name of the theme
    QString name;

    bool custom{};
};

class Theme final
{
public:
    static const std::vector<ThemeDescriptor> builtInThemes;

    // The built in theme that will be used if some theme parsing fails
    static const ThemeDescriptor fallbackTheme;

    static const int AUTO_RELOAD_INTERVAL_MS = 500;

    Theme(const Paths &paths);

    bool isLightTheme() const;
    bool isSystemTheme() const;

    struct TabColors {
        QColor text;
        struct Backgrounds {
            QColor regular;
            QColor hover;
            QColor unfocused;
        };
        Backgrounds backgrounds;
        struct Line {
            QColor regular;
            QColor hover;
            QColor unfocused;
        };
        Line line;
    };

    QColor accent{"#00aeef"};

    /// WINDOW
    struct Window {
        QColor background;
        QColor text;
    };
    Window window;

    /// TABS
    struct Tabs {
        TabColors regular;
        TabColors newMessage;
        TabColors highlighted;
        TabColors selected;
        QColor dividerLine;

        QColor liveIndicator;
        QColor rerunIndicator;
    };
    Tabs tabs;

    /// MESSAGES
    struct Messages {
        struct TextColors {
            QColor regular;
            QColor caret;
            QColor link;
            QColor system;
            QColor chatPlaceholder;
        };
        TextColors textColors;

        struct Backgrounds {
            QColor regular;
            QColor alternate;
        };
        Backgrounds backgrounds;

        QColor disabled;
        QColor selection;

        QColor highlightAnimationStart;
        QColor highlightAnimationEnd;
    };
    Messages messages;

    /// SCROLLBAR
    struct Scrollbars {
        QColor background;
        QColor thumb;
        QColor thumbSelected;
    };
    Scrollbars scrollbars;

    /// SPLITS
    struct Splits {
        QColor messageSeperator;
        QColor background;
        QColor dropPreview;
        QColor dropPreviewBorder;
        QColor dropTargetRect;
        QColor dropTargetRectBorder;
        QColor resizeHandle;
        QColor resizeHandleBackground;

        struct Header {
            QColor border;
            QColor focusedBorder;
            QColor background;
            QColor focusedBackground;
            QColor text;
            QColor focusedText;
        };
        Header header;

        struct Input {
            QColor background;
            QColor text;
            QString styleSheet;
        };
        Input input;
    };
    Splits splits;

    struct {
        QPixmap copy;
        QPixmap pin;
    } buttons;

    void normalizeColor(QColor &color) const;
    void update();

    bool isAutoReloading() const;
    void setAutoReload(bool autoReload);

    /**
     * Return a list of available themes
     **/
    std::vector<std::pair<QString, QVariant>> availableThemes() const;

    pajlada::Signals::NoArgSignal updated;

    QStringSetting themeName{"/appearance/theme/name", "Dark"};
    QStringSetting lightSystemThemeName{"/appearance/theme/lightSystem",
                                        "Light"};
    QStringSetting darkSystemThemeName{"/appearance/theme/darkSystem", "Dark"};

private:
    bool isLight_ = false;

    std::vector<ThemeDescriptor> availableThemes_;

    QString currentThemePath_;
    std::unique_ptr<QTimer> themeReloadTimer_;
    // This will only be populated when auto-reloading themes
    QJsonObject currentThemeJson_;

    QObject lifetime_;

    /**
     * Figure out which themes are available in the Themes directory
     *
     * NOTE: This is currently not built to be reloadable
     **/
    void loadAvailableThemes(const Paths &paths);

    std::optional<ThemeDescriptor> findThemeByKey(const QString &key);

    void parseFrom(const QJsonObject &root, bool isCustomTheme);

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets_;

    friend class WindowManager;
};

Theme *getTheme();

namespace theme::detail {

    // from Boost PFR (fake_object.hpp)
    // Used to create a reference to an object at constant evaluation
    template <class T>
    struct Wrapper {
        const T value;
    };

    template <class T>
    extern const Wrapper<T> EXTERN_WRAPPER;
    template <class T>
    consteval const T &fakeObject() noexcept
    {
        return EXTERN_WRAPPER<T>.value;
    }

    template <auto Ptr>
    constexpr bool IGNORE_DESER = false;

    template <>
    constexpr bool IGNORE_DESER<std::addressof(
        fakeObject<Theme::Splits::Input>().styleSheet)> = true;

}  // namespace theme::detail

}  // namespace chatterino
