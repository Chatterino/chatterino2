#include "widgets/AccountSwitchPopup.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"

#include <QHBoxLayout>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {

AccountSwitchPopup::AccountSwitchPopup(QWidget *parent)
    : BaseWindow({BaseWindow::TopMost, BaseWindow::Frameless}, parent)
{
#ifdef Q_OS_LINUX
    this->setWindowFlag(Qt::Popup);
#endif

    this->setContentsMargins(0, 0, 0, 0);

    this->ui_.accountSwitchWidget = new AccountSwitchWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    this->ui_.accountSwitchWidget->setFocusPolicy(Qt::NoFocus);
    vbox->addWidget(this->ui_.accountSwitchWidget);

    auto hbox = new QHBoxLayout();
    auto manageAccountsButton = new QPushButton(this);
    manageAccountsButton->setText("Manage Accounts");
    manageAccountsButton->setFocusPolicy(Qt::NoFocus);
    hbox->addWidget(manageAccountsButton);
    vbox->addLayout(hbox);

    connect(manageAccountsButton, &QPushButton::clicked, [this]() {
        SettingsDialog::showDialog(this, SettingsDialogPreference::Accounts);
    });

    this->getLayoutContainer()->setLayout(vbox);

    this->setScaleIndependantSize(200, 200);
}

void AccountSwitchPopup::refresh()
{
    this->ui_.accountSwitchWidget->refresh();
}

void AccountSwitchPopup::focusOutEvent(QFocusEvent *)
{
    this->hide();
}

void AccountSwitchPopup::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setPen(QColor("#999"));
    painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
}

}  // namespace chatterino
