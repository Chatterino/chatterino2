#include "widgets/AccountSwitchWidget.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

AccountSwitchWidget::AccountSwitchWidget(QWidget *parent)
    : QListWidget(parent)
{
    auto *app = getApp();

    this->addItem(ANONYMOUS_USERNAME_LABEL);

    for (const auto &userName : app->getAccounts()->twitch.getUsernames())
    {
        this->addItem(userName);
    }

    this->managedConnections_.managedConnect(
        app->getAccounts()->twitch.userListUpdated, [=, this]() {
            this->blockSignals(true);

            this->clear();

            this->addItem(ANONYMOUS_USERNAME_LABEL);

            for (const auto &userName :
                 app->getAccounts()->twitch.getUsernames())
            {
                this->addItem(userName);
            }

            this->refreshSelection();

            this->blockSignals(false);
        });

    this->refreshSelection();

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
