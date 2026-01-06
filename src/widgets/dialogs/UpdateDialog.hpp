// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "singletons/Updates.hpp"
#include "widgets/BaseWindow.hpp"

#include <pajlada/signals/signalholder.hpp>

class QPushButton;

namespace chatterino {

class Label;

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
