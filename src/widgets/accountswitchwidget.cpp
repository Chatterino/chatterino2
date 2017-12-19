#include "accountswitchwidget.hpp"
#include "accountmanager.hpp"

namespace chatterino {
namespace widgets {

AccountSwitchWidget::AccountSwitchWidget(QWidget *parent)
    : QListWidget(parent)
{
    static QString anonUsername(" - anonymous - ");

    this->addItem(anonUsername);

    for (const auto &userName : AccountManager::getInstance().Twitch.getUsernames()) {
        this->addItem(userName);
    }

    AccountManager::getInstance().Twitch.userListUpdated.connect([this]() {
        this->blockSignals(true);

        this->clear();

        this->addItem(anonUsername);

        for (const auto &userName : AccountManager::getInstance().Twitch.getUsernames()) {
            this->addItem(userName);
        }

        this->refreshSelection();

        this->blockSignals(false);
    });

    this->refreshSelection();

    QObject::connect(this, &QListWidget::clicked, [this] {
        if (!this->selectedItems().isEmpty()) {
            QString newUsername = this->currentItem()->text();
            if (newUsername.compare(anonUsername, Qt::CaseInsensitive) == 0) {
                AccountManager::getInstance().Twitch.currentUsername = "";
            } else {
                AccountManager::getInstance().Twitch.currentUsername = newUsername.toStdString();
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
        auto currentUser = AccountManager::getInstance().Twitch.getCurrent();

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
