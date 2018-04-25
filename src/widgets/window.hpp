#pragma once

#include "util/helpers.hpp"
#include "widgets/basewindow.hpp"
#include "widgets/notebook.hpp"

//#ifdef USEWINSDK
//#include <platform/borderless/qwinwidget.h>
//#endif

#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace singletons {
class ThemeManager;
}  // namespace singletons

namespace widgets {

class Window : public BaseWindow
{
    Q_OBJECT

public:
    enum WindowType { Main, Popup, Attached };

    explicit Window(singletons::ThemeManager &_themeManager, WindowType type);

    void repaintVisibleChatWidgets(Channel *channel = nullptr);

    Notebook &getNotebook();

    void refreshWindowTitle(const QString &username);

    pajlada::Signals::NoArgSignal closed;

    WindowType getType();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *event) override;

private:
    WindowType type;
    float dpi;

    void loadGeometry();

    Notebook notebook;

    friend class Notebook;

public:
    void save();
};

}  // namespace widgets
}  // namespace chatterino
