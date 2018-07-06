#pragma once

#include "util/Helpers.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/dialogs/UpdateDialog.hpp"

//#ifdef USEWINSDK
//#include <platform/borderless/qwinwidget.h>
//#endif

#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class Theme;

class Window : public BaseWindow
{
    Q_OBJECT

public:
    enum class Type { Main, Popup, Attached };

    explicit Window(Window::Type type);

    Type getType();
    SplitNotebook &getNotebook();

    void repaintVisibleChatWidgets(Channel *channel = nullptr);

    pajlada::Signals::NoArgSignal closed;

protected:
    void showEvent(QShowEvent *) override;
    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *event) override;

private:
    void addCustomTitlebarButtons();
    void addDebugStuff();
    void addShortcuts();
    void addLayout();
    void onAccountSelected();

    Type type_;

    SplitNotebook notebook_;
    RippleEffectLabel *userLabel_ = nullptr;
    std::unique_ptr<UpdateDialog> updateDialogHandle_;

    pajlada::Signals::SignalHolder signalHolder_;

    friend class Notebook;
};

}  // namespace chatterino
