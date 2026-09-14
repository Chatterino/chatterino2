// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/AccountsPage.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/accounts/AccountModel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/LoginDialog.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/helper/ForegroundItemDelegate.hpp"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>

namespace chatterino {

AccountsPage::AccountsPage()
{
    auto *app = getApp();

    LayoutCreator<AccountsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    auto *model = app->getAccounts()->createModel(nullptr);

    EditableModelView *view =
        layout.emplace<EditableModelView>(model, false).getElement();

    this->signalHolder_.managedConnect(
        app->getAccounts()->twitch.loginExpiryChanged, [model] {
            model->refreshExpiredState();
        });

    // The expired marker is a themed color, so it has to be re-applied
    this->signalHolder_.managedConnect(getTheme()->updated, [model] {
        model->refreshExpiredState();
    });

    view->getTableView()->horizontalHeader()->setVisible(false);
    view->getTableView()->horizontalHeader()->setStretchLastSection(true);

    // Keeps the expired marker red while that account's row is selected
    view->getTableView()->setItemDelegate(
        new ForegroundItemDelegate(view->getTableView()));

    // We can safely ignore this signal connection since we own the view
    std::ignore = view->addButtonPressed.connect([this] {
        LoginDialog d(this);
        d.exec();
    });

    view->getTableView()->setStyleSheet("background: #333");

    //    auto buttons = layout.emplace<QDialogButtonBox>();
    //    {
    //        this->addButton = buttons->addButton("Add",
    //        QDialogButtonBox::YesRole); this->removeButton =
    //        buttons->addButton("Remove", QDialogButtonBox::NoRole);
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

    //        getApp()->getAccounts()->Twitch.removeUser(selectedUser);
    //    });
}

}  // namespace chatterino
