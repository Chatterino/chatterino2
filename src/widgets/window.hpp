#pragma once

#include "util/helpers.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/notebook.hpp"
#include "widgets/titlebar.hpp"

//#ifdef USEWINSDK
//#include <platform/borderless/qwinwidget.h>
//#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/signals2.hpp>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class ChannelManager;
class ColorScheme;
class CompletionManager;

namespace widgets {

struct WindowGeometry {
    WindowGeometry(const std::string &key)
        : x(fS("/windows/{}/geometry/x", key))
        , y(fS("/windows/{}/geometry/y", key))
        , width(fS("/windows/{}/geometry/width", key))
        , height(fS("/windows/{}/geometry/height", key))
    {
    }

    pajlada::Settings::Setting<int> x;
    pajlada::Settings::Setting<int> y;
    pajlada::Settings::Setting<int> width;
    pajlada::Settings::Setting<int> height;
};

class Window : public BaseWidget
{
    Q_OBJECT

    QString windowName;

    WindowGeometry windowGeometry;

public:
    explicit Window(const QString &_windowName, ChannelManager &_channelManager,
                    ColorScheme &_colorScheme, bool isMainWindow);

    void repaintVisibleChatWidgets(Channel *channel = nullptr);

    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
    void loadDefaults();

    bool isLoaded() const;

    Notebook &getNotebook();

    void refreshWindowTitle(const QString &username);

    boost::signals2::signal<void()> closed;

    pajlada::Signals::NoArgSignal lostFocus;

protected:
    virtual void closeEvent(QCloseEvent *event) override;

    virtual void changeEvent(QEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
private:
    float dpi;

    virtual void refreshTheme() override;

    void loadGeometry();

    ChannelManager &channelManager;
    ColorScheme &colorScheme;

    Notebook notebook;
    bool loaded = false;
    TitleBar titleBar;

    friend class Notebook;
};

}  // namespace widgets
}  // namespace chatterino
