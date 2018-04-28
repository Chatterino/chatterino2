#include "accountspage.hpp"

#include "application.hpp"
#include "const.hpp"
#include "singletons/accountmanager.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/logindialog.hpp"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {
namespace settingspages {

AccountsPage::AccountsPage()
    : SettingsPage("Accounts", ":/images/accounts.svg")
{
    util::LayoutCreator<AccountsPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    auto buttons = layout.emplace<QDialogButtonBox>();
    {
        this->addButton = buttons->addButton("Add", QDialogButtonBox::YesRole);
        this->removeButton = buttons->addButton("Remove", QDialogButtonBox::NoRole);
    }

    layout.emplace<AccountSwitchWidget>(this).assign(&this->accSwitchWidget);

    // ----
    QObject::connect(this->addButton, &QPushButton::clicked, []() {
        static auto loginWidget = new LoginWidget();
        loginWidget->show();
    });

    QObject::connect(this->removeButton, &QPushButton::clicked, [this] {
        auto selectedUser = this->accSwitchWidget->currentItem()->text();
        if (selectedUser == ANONYMOUS_USERNAME_LABEL) {
            // Do nothing
            return;
        }

        getApp()->accounts->Twitch.removeUser(selectedUser);
    });
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
