#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "buttons/SvgButton.hpp"
#    include "widgets/BaseWindow.hpp"

#    include <boost/signals2/connection.hpp>
#    include <QString>
#    include <QTextBlockFormat>
#    include <QTextCharFormat>
#    include <sol/forward.hpp>

class QTextEdit;
class QTextCharFormat;
class QTextBlockFormat;

namespace chatterino::lua::api {
enum class LogLevel;
}  // namespace chatterino::lua::api

namespace chatterino {

class Plugin;

class PluginRepl : public BaseWindow
{
public:
    PluginRepl(QString id, QWidget *parent = nullptr);

    static QFont currentFont();

protected:
    void themeChangedEvent() override;

private:
    struct LogOptions {
        /// Maximum number of items to show in tables.
        size_t maxItems = 10;
    };

    void tryRun(QString code);
    void logResult(const sol::protected_function_result &res,
                   const LogOptions &opts);

    void log(std::optional<lua::api::LogLevel> level, const QString &text);

    void tryUpdate();
    void setPlugin(Plugin *plugin);

    void updateFont();
    void updatePinned();

    QString id;
    Plugin *plugin = nullptr;

    boost::signals2::scoped_connection pluginDestroyConn;
    boost::signals2::scoped_connection pluginLogConn;
    boost::signals2::scoped_connection pluginLoadedConn;

    bool isPinned = false;

    struct {
        QTextEdit *input = nullptr;
        QTextEdit *output = nullptr;
        SvgButton *clear = nullptr;
        SvgButton *reload = nullptr;
        SvgButton *pin = nullptr;
        SvgButton::Src pinDisabledSource_{
            .dark = ":/buttons/pinDisabled-darkMode.svg",
            .light = ":/buttons/pinDisabled-lightMode.svg",
        };
        SvgButton::Src pinEnabledSource_{
            .dark = ":/buttons/pinEnabled.svg",
            .light = ":/buttons/pinEnabled.svg",
        };
    } ui;

    struct {
        QTextCharFormat self;
        QTextCharFormat debug;
        QTextCharFormat info;
        QTextCharFormat warning;
        QTextCharFormat error;
    } charFormats;
    struct {
        QTextBlockFormat self;
        QTextBlockFormat debug;
        QTextBlockFormat info;
        QTextBlockFormat warning;
        QTextBlockFormat error;
    } blockFormats;

    QFont font;
};

}  // namespace chatterino

#endif
