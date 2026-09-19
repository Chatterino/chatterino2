// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class AccountSwitchWidget;

class AccountsPage : public SettingsPage
{
public:
    AccountsPage();

private:
    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
