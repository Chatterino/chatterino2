#include "accountspage.hpp"

#include "application.hpp"
#include "const.hpp"
#include "controllers/accounts/accountcontroller.hpp"
#include "controllers/accounts/accountmodel.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/helper/editablemodelview.hpp"
#include "widgets/logindialog.hpp"

#include <algorithm>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {
namespace settingspages {

AccountsPage::AccountsPage()
    : SettingsPage("Accounts", ":/images/accounts.svg")
{
    auto *app = getApp();

    util::LayoutCreator<AccountsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    helper::EditableModelView *view =
        *layout.emplace<helper::EditableModelView>(app->accounts->createModel(nullptr));

    view->setTitles({"Name"});
    view->getTableView()->horizontalHeader()->setStretchLastSection(true);

    view->addButtonPressed.connect([] {
        static auto loginWidget = new LoginWidget();
        loginWidget->show();
    });

    //    auto buttons = layout.emplace<QDialogButtonBox>();
    //    {
    //        this->addButton = buttons->addButton("Add", QDialogButtonBox::YesRole);
    //        this->removeButton = buttons->addButton("Remove", QDialogButtonBox::NoRole);
    //    }

    //    layout.emplace<AccountSwitchWidget>(this).assign(&this->accSwitchWidget);

    // ----
    //    QObject::connect(this->addButton, &QPushButton::clicked, []() {
    //        static auto loginWidget = new LoginWidget();
    //        loginWidget->show();
    //    });

    //    QObject::connect(this->removeButton, &QPushButton::clicked, [this] {
    //        auto selectedUser = this->accSwitchWidget->currentItem()->text();
    //        if (selectedUser == ANONYMOUS_USERNAME_LABEL) {
    //            // Do nothing
    //            return;
    //        }

    //        getApp()->accounts->Twitch.removeUser(selectedUser);
    //    });
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
