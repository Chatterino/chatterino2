#pragma once

#include "util/Helpers.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/Notebook.hpp"

//#ifdef USEWINSDK
//#include <platform/borderless/qwinwidget.h>
//#endif

#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class ThemeManager;

class Window : public BaseWindow
{
    Q_OBJECT

public:
    enum WindowType { Main, Popup, Attached };

    explicit Window(WindowType type);

    void repaintVisibleChatWidgets(Channel *channel = nullptr);

    SplitNotebook &getNotebook();

    void refreshWindowTitle(const QString &username);

    pajlada::Signals::NoArgSignal closed;

    WindowType getType();

protected:
    void showEvent(QShowEvent *) override;
    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *event) override;

private:
    RippleEffectLabel *userLabel = nullptr;

    WindowType type;
    float dpi;

    void loadGeometry();

    SplitNotebook notebook;

    friend class Notebook;

public:
    void save();
};

}  // namespace chatterino
