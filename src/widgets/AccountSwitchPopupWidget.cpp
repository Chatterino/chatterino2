#include "widgets/AccountSwitchPopupWidget.hpp"
#include "util/Log.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"

#include <QHBoxLayout>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino
{
    AccountSwitchPopupWidget::AccountSwitchPopupWidget(QWidget* parent)
        : QWidget(parent)
    {
        this->setWindowFlags(
            Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

        this->setContentsMargins(0, 0, 0, 0);

        this->ui_.accountSwitchWidget = new AccountSwitchWidget(this);
        QVBoxLayout* vbox = new QVBoxLayout(this);
        this->ui_.accountSwitchWidget->setFocusPolicy(Qt::NoFocus);
        vbox->addWidget(this->ui_.accountSwitchWidget);

        // vbox->setSizeConstraint(QLayout::SetMinimumSize);

        auto hbox = new QHBoxLayout();
        auto manageAccountsButton = new QPushButton(this);
        manageAccountsButton->setText("Manage Accounts");
        hbox->addWidget(manageAccountsButton);
        vbox->addLayout(hbox);

        connect(manageAccountsButton, &QPushButton::clicked, []() {
            SettingsDialog::showDialog(SettingsDialogPreference::Accounts);  //
        });

        this->setLayout(vbox);

        //    this->setStyleSheet("background: #333");
    }

    void AccountSwitchPopupWidget::refresh()
    {
        this->ui_.accountSwitchWidget->refresh();
    }

    void AccountSwitchPopupWidget::focusOutEvent(QFocusEvent*)
    {
        this->hide();
    }

    void AccountSwitchPopupWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        painter.setPen(QColor("#999"));
        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }
}  // namespace chatterino
