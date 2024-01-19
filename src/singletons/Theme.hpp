#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/Singleton.hpp"
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

class Theme final : public Singleton
{
public:
    static const std::vector<ThemeDescriptor> builtInThemes;

    // The built in theme that will be used if some theme parsing fails
    static const ThemeDescriptor fallbackTheme;

    static const int AUTO_RELOAD_INTERVAL_MS = 500;

    void initialize(Settings &settings, const Paths &paths) final;

    bool isLightTheme() const;

    struct TabColors {
        QColor text;
        struct {
            QColor regular;
            QColor hover;
            QColor unfocused;
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
    } window;

    /// TABS
    struct {
        TabColors regular;
        TabColors newMessage;
        TabColors highlighted;
        TabColors selected;
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
        } backgrounds;

        QColor disabled;
        QColor selection;

        QColor highlightAnimationStart;
        QColor highlightAnimationEnd;
    } messages;

    /// SCROLLBAR
    struct {
        QColor background;
        QColor thumb;
        QColor thumbSelected;
    } scrollbars;

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
            QColor focusedBorder;
            QColor background;
            QColor focusedBackground;
            QColor text;
            QColor focusedText;
        } header;

        struct {
            QColor background;
            QColor text;
            QString styleSheet;
        } input;
    } splits;

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

private:
    bool isLight_ = false;

    std::vector<ThemeDescriptor> availableThemes_;

    QString currentThemePath_;
    std::unique_ptr<QTimer> themeReloadTimer_;
    // This will only be populated when auto-reloading themes
    QJsonObject currentThemeJson_;

    /**
     * Figure out which themes are available in the Themes directory
     *
     * NOTE: This is currently not built to be reloadable
     **/
    void loadAvailableThemes(const Paths &paths);

    std::optional<ThemeDescriptor> findThemeByKey(const QString &key);

    void parseFrom(const QJsonObject &root);

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets_;

    friend class WindowManager;
};

Theme *getTheme();
}  // namespace chatterino
