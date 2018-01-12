#pragma once

#include <QPushButton>

#include "widgets/accountswitchwidget.hpp"
#include "widgets/settingspages/settingspage.hpp"

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
