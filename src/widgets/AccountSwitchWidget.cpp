// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/AccountSwitchWidget.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "widgets/helper/ForegroundItemDelegate.hpp"

namespace chatterino {

AccountSwitchWidget::AccountSwitchWidget(QWidget *parent)
    : QListWidget(parent)
{
    auto *app = getApp();

    // Keeps the expired marker red while that account is the selected one
    this->setItemDelegate(new ForegroundItemDelegate(this));

    this->refreshList();

    this->managedConnections_.managedConnect(
        app->getAccounts()->twitch.userListUpdated, [this]() {
            this->refreshList();
        });

    this->managedConnections_.managedConnect(
        app->getAccounts()->twitch.loginExpiryChanged, [this]() {
            this->refreshList();
        });

    // The expired marker is a themed color, so it has to be re-applied
    this->managedConnections_.managedConnect(getTheme()->updated, [this]() {
        this->refreshList();
    });

    QObject::connect(this, &QListWidget::clicked, [=, this] {
        if (!this->selectedItems().isEmpty())
        {
            QString newUsername = this->currentItem()->text();
            if (newUsername.compare(ANONYMOUS_USERNAME_LABEL,
                                    Qt::CaseInsensitive) == 0)
            {
                app->getAccounts()->twitch.currentUsername = "";
            }
            else
            {
                app->getAccounts()->twitch.currentUsername = newUsername;
            }

            getSettings()->requestSave();
        }
    });
}

void AccountSwitchWidget::refresh()
{
    this->refreshSelection();
}

void AccountSwitchWidget::refreshList()
{
    this->blockSignals(true);

    this->clear();

    this->addAccountItem(ANONYMOUS_USERNAME_LABEL, false);

    for (const auto &userName : getApp()->getAccounts()->twitch.getUsernames())
    {
        auto account =
            getApp()->getAccounts()->twitch.findUserByUsername(userName);
        this->addAccountItem(userName, account && account->isExpired());
    }

    this->refreshSelection();

    this->blockSignals(false);
}

void AccountSwitchWidget::addAccountItem(const QString &userName, bool expired)
{
    auto *item = new QListWidgetItem(userName, this);

    if (expired)
    {
        // Only the color marks this - spelling it out would widen the popup
        // enough to make it scroll for long usernames
        item->setForeground(getTheme()->accounts.expired);
        item->setToolTip(
            "This account's login has expired - sign in again to use it");
    }
}

void AccountSwitchWidget::refreshSelection()
{
    this->blockSignals(true);

    // Select the currently logged in user
    if (this->count() > 0)
    {
        auto *app = getApp();

        auto currentUser = app->getAccounts()->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            this->setCurrentRow(0);
        }
        else
        {
            const QString &currentUsername = currentUser->getUserName();
            for (int i = 0; i < this->count(); ++i)
            {
                QString itemText = this->item(i)->text();

                if (itemText.compare(currentUsername, Qt::CaseInsensitive) == 0)
                {
                    this->setCurrentRow(i);
                    break;
                }
            }
        }
    }

    this->blockSignals(false);
}

}  // namespace chatterino
