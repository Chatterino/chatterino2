#pragma once

#include "widgets/BaseWindow.hpp"

#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class Theme;
class UpdateDialog;
class SplitNotebook;
class Channel;

enum class WindowType { Main, Popup, Attached };

class Window : public BaseWindow
{
    Q_OBJECT

public:
    explicit Window(WindowType type);

    WindowType getType();
    SplitNotebook &getNotebook();

    pajlada::Signals::NoArgSignal closed;

protected:
    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *event) override;

private:
    void addCustomTitlebarButtons();
    void addDebugStuff(
        std::map<QString, std::function<QString(std::vector<QString>)>>
            &actions);
    virtual void addShortcuts() override;
    void addLayout();
    void onAccountSelected();
    void addMenuBar();

    WindowType type_;

    SplitNotebook *notebook_;
    EffectLabel *userLabel_ = nullptr;
    std::shared_ptr<UpdateDialog> updateDialogHandle_;

    pajlada::Signals::SignalHolder signalHolder_;

    friend class Notebook;
};

}  // namespace chatterino
