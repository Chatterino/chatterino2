#pragma once

#include "util/Helpers.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/dialogs/UpdatePromptDialog.hpp"

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
    void addCustomTitlebarButtons();
    void loadGeometry();

    RippleEffectLabel *userLabel = nullptr;
    std::unique_ptr<UpdatePromptDialog> updateDialogHandle_;

    WindowType type;
    float dpi;

    SplitNotebook notebook;

    pajlada::Signals::SignalHolder signalHolder;

    friend class Notebook;

public:
    void save();
};

}  // namespace chatterino
