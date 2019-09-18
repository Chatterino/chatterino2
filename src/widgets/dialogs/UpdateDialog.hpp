#pragma once

#include "pajlada/signals/signalholder.hpp"
#include "singletons/Updates.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/Label.hpp"

class QPushButton;

namespace chatterino {

class UpdateDialog : public BaseWindow
{
public:
    enum Button { Dismiss, Install };

    UpdateDialog();

    pajlada::Signals::Signal<Button> buttonClicked;

private:
    void updateStatusChanged(Updates::Status status);

    struct {
        Label *label = nullptr;
        QPushButton *installButton = nullptr;
    } ui_;

    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
