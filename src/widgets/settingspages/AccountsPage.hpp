#pragma once

#include "widgets/accountswitchwidget.hpp"
#include "widgets/settingspages/settingspage.hpp"

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
