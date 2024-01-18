#include "widgets/AccountSwitchPopup.hpp"

#include "common/Literals.hpp"
#include "singletons/Theme.hpp"
#include "widgets/AccountSwitchWidget.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"

#include <QLayout>
#include <QPainter>
#include <QPushButton>

namespace chatterino {

using namespace literals;

AccountSwitchPopup::AccountSwitchPopup(QWidget *parent)
    : BaseWindow({BaseWindow::TopMost, BaseWindow::Frameless,
                  BaseWindow::DisableLayoutSave},
                 parent)
{
#ifdef Q_OS_LINUX
    this->setWindowFlag(Qt::Popup);
#endif

    this->setContentsMargins(0, 0, 0, 0);

    this->ui_.accountSwitchWidget = new AccountSwitchWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    this->ui_.accountSwitchWidget->setFocusPolicy(Qt::NoFocus);
    vbox->addWidget(this->ui_.accountSwitchWidget);

    auto *hbox = new QHBoxLayout();
    auto *manageAccountsButton = new QPushButton(this);
    manageAccountsButton->setText("Manage Accounts");
    manageAccountsButton->setFocusPolicy(Qt::NoFocus);
    hbox->addWidget(manageAccountsButton);
    vbox->addLayout(hbox);

    connect(manageAccountsButton, &QPushButton::clicked, [this]() {
        SettingsDialog::showDialog(this, SettingsDialogPreference::Accounts);
    });

    this->getLayoutContainer()->setLayout(vbox);

    this->setScaleIndependantSize(200, 200);
    this->themeChangedEvent();
}

void AccountSwitchPopup::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    auto *t = getTheme();
    auto color = [](const QColor &c) {
        return c.name(QColor::HexArgb);
    };
    this->setStyleSheet(uR"(
        QListView {
            color: %1;
            background: %2;
        }
        QListView::item:hover {
            background: %3;
        }
        QListView::item:selected {
            background: %4;
        }

        QPushButton {
            background: %5;
            color: %1;
        }
        QPushButton:hover {
            background: %3;
        }
        QPushButton:pressed {
            background: %6;
        }

        chatterino--AccountSwitchPopup {
            background: %7;
        }
    )"_s.arg(color(t->window.text), color(t->splits.header.background),
             color(t->splits.header.focusedBackground), color(t->accent),
             color(t->tabs.regular.backgrounds.regular),
             color(t->tabs.selected.backgrounds.regular),
             color(t->window.background)));
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
