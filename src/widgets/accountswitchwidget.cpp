#include "accountswitchwidget.hpp"
#include "const.hpp"
#include "singletons/accountmanager.hpp"

namespace chatterino {
namespace widgets {

AccountSwitchWidget::AccountSwitchWidget(QWidget *parent)
    : QListWidget(parent)
{
    singletons::AccountManager &accountManager = singletons::AccountManager::getInstance();

    this->addItem(ANONYMOUS_USERNAME_LABEL);

    for (const auto &userName : accountManager.Twitch.getUsernames()) {
        this->addItem(userName);
    }

    accountManager.Twitch.userListUpdated.connect([this, &accountManager]() {
        this->blockSignals(true);

        this->clear();

        this->addItem(ANONYMOUS_USERNAME_LABEL);

        for (const auto &userName : accountManager.Twitch.getUsernames()) {
            this->addItem(userName);
        }

        this->refreshSelection();

        this->blockSignals(false);
    });

    this->refreshSelection();

    QObject::connect(this, &QListWidget::clicked, [this, &accountManager] {
        if (!this->selectedItems().isEmpty()) {
            QString newUsername = this->currentItem()->text();
            if (newUsername.compare(ANONYMOUS_USERNAME_LABEL, Qt::CaseInsensitive) == 0) {
                accountManager.Twitch.currentUsername = "";
            } else {
                accountManager.Twitch.currentUsername = newUsername.toStdString();
            }
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
    if (this->count() > 0) {
        auto currentUser = singletons::AccountManager::getInstance().Twitch.getCurrent();

        if (currentUser->isAnon()) {
            this->setCurrentRow(0);
        } else {
            const QString &currentUsername = currentUser->getUserName();
            for (int i = 0; i < this->count(); ++i) {
                QString itemText = this->item(i)->text();

                if (itemText.compare(currentUsername, Qt::CaseInsensitive) == 0) {
                    this->setCurrentRow(i);
                    break;
                }
            }
        }
    }

    this->blockSignals(false);
}

}  // namespace widgets
}  // namespace chatterino
