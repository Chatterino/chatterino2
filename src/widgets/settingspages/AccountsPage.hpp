#pragma once

#include "widgets/AccountSwitchWidget.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QPushButton>

namespace chatterino {
namespace widgets {
namespace settingspages {

class AccountsPage : public SettingsPage
{
public:
    AccountsPage();

private:
    QPushButton *addButton;
    QPushButton *removeButton;
    AccountSwitchWidget *accSwitchWidget;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
