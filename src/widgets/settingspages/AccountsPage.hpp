#pragma once

#include "widgets/AccountSwitchWidget.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QPushButton>

namespace chatterino {

class AccountsPage : public SettingsPage
{
public:
    AccountsPage();

private:
    QPushButton *addButton_;
    QPushButton *removeButton_;
    AccountSwitchWidget *accountSwitchWidget_;
};

}  // namespace chatterino
