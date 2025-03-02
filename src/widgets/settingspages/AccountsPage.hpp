#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QPushButton>

namespace chatterino {

class AccountSwitchWidget;

class EditableModelView;

class AccountsPage : public SettingsPage
{
public:
    AccountsPage();
    bool filterElements(const QString &query) override;

private:
    QPushButton *addButton_{};
    QPushButton *removeButton_{};
    AccountSwitchWidget *accountSwitchWidget_{};
    EditableModelView *view_;
};

}  // namespace chatterino
