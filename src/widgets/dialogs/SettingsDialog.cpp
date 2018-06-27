#include "widgets/dialogs/SettingsDialog.hpp"

#include "Application.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/SettingsDialogTab.hpp"
#include "widgets/settingspages/AboutPage.hpp"
#include "widgets/settingspages/AccountsPage.hpp"
#include "widgets/settingspages/AppearancePage.hpp"
#include "widgets/settingspages/BehaviourPage.hpp"
#include "widgets/settingspages/BrowserExtensionPage.hpp"
#include "widgets/settingspages/CommandPage.hpp"
#include "widgets/settingspages/EmotesPage.hpp"
#include "widgets/settingspages/ExternalToolsPage.hpp"
#include "widgets/settingspages/HighlightingPage.hpp"
#include "widgets/settingspages/IgnoreusersPage.hpp"
#include "widgets/settingspages/KeyboardSettingsPage.hpp"
#include "widgets/settingspages/LogsPage.hpp"
#include "widgets/settingspages/ModerationPage.hpp"
#include "widgets/settingspages/SpecialChannelsPage.hpp"

#include <QDialogButtonBox>

namespace chatterino {

SettingsDialog *SettingsDialog::handle = nullptr;

SettingsDialog::SettingsDialog()
    : BaseWindow(nullptr, BaseWindow::DisableCustomScaling)
{
    this->initUi();

    this->addTabs();

    this->scaleChangedEvent(this->getScale());
}

void SettingsDialog::initUi()
{
    LayoutCreator<SettingsDialog> layoutCreator(this);

    // tab pages
    layoutCreator.emplace<QWidget>()
        .assign(&this->ui_.tabContainerContainer)
        .emplace<QVBoxLayout>()
        .withoutMargin()
        .assign(&this->ui_.tabContainer);

    // right side layout
    auto right = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        right.emplace<QStackedLayout>().assign(&this->ui_.pageStack).withoutMargin();

        auto buttons = right.emplace<QDialogButtonBox>(Qt::Horizontal);
        {
            this->ui_.okButton = buttons->addButton("Ok", QDialogButtonBox::YesRole);
            this->ui_.cancelButton = buttons->addButton("Cancel", QDialogButtonBox::NoRole);
        }
    }

    // ---- misc
    this->ui_.tabContainerContainer->setObjectName("tabWidget");
    this->ui_.pageStack->setObjectName("pages");

    QObject::connect(this->ui_.okButton, &QPushButton::clicked, this,
                     &SettingsDialog::okButtonClicked);
    QObject::connect(this->ui_.cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::cancelButtonClicked);
}

SettingsDialog *SettingsDialog::getHandle()
{
    return SettingsDialog::handle;
}

void SettingsDialog::addTabs()
{
    this->ui_.tabContainer->setSpacing(0);

    this->addTab(new AccountsPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new AppearancePage);
    this->addTab(new BehaviourPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new CommandPage);
    //    this->addTab(new EmotesPage);
    this->addTab(new HighlightingPage);
    this->addTab(new IgnoreUsersPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new KeyboardSettingsPage);
    //    this->addTab(new LogsPage);
    this->addTab(new ModerationPage);
    //    this->addTab(new SpecialChannelsPage);
    this->addTab(new BrowserExtensionPage);
    this->addTab(new ExternalToolsPage);

    this->ui_.tabContainer->addStretch(1);
    this->addTab(new AboutPage, Qt::AlignBottom);
}

void SettingsDialog::addTab(SettingsPage *page, Qt::Alignment alignment)
{
    auto tab = new SettingsDialogTab(this, page, page->getIconResource());

    this->ui_.pageStack->addWidget(page);
    this->ui_.tabContainer->addWidget(tab, 0, alignment);
    this->tabs.push_back(tab);

    if (this->tabs.size() == 1) {
        this->select(tab);
    }
}

void SettingsDialog::select(SettingsDialogTab *tab)
{
    this->ui_.pageStack->setCurrentWidget(tab->getSettingsPage());

    if (this->selectedTab != nullptr) {
        this->selectedTab->setSelected(false);
        this->selectedTab->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #555; color: #FFF");
    this->selectedTab = tab;
}

void SettingsDialog::showDialog(PreferredTab preferredTab)
{
    static SettingsDialog *instance = new SettingsDialog();
    instance->refresh();

    switch (preferredTab) {
        case SettingsDialog::PreferredTab::Accounts: {
            instance->select(instance->tabs.at(0));
        } break;
    }

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void SettingsDialog::refresh()
{
    //    this->ui.accountSwitchWidget->refresh();

    getApp()->settings->saveSnapshot();

    for (auto *tab : this->tabs) {
        tab->getSettingsPage()->onShow();
    }
}

void SettingsDialog::scaleChangedEvent(float newDpi)
{
    QFile file(":/qss/settings.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    styleSheet.replace("<font-size>", QString::number((int)(14 * newDpi)));
    styleSheet.replace("<checkbox-size>", QString::number((int)(14 * newDpi)));

    for (SettingsDialogTab *tab : this->tabs) {
        tab->setFixedHeight((int)(30 * newDpi));
    }

    this->setStyleSheet(styleSheet);

    this->ui_.tabContainerContainer->setFixedWidth((int)(200 * newDpi));
}

void SettingsDialog::themeRefreshEvent()
{
    BaseWindow::themeRefreshEvent();

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    this->setPalette(palette);
}

// void SettingsDialog::setChildrensFont(QLayout *object, QFont &font, int indent)
//{
//    //    for (QWidget *widget : this->widgets) {
//    //        widget->setFont(font);
//    //    }
//    //    for (int i = 0; i < object->count(); i++) {
//    //        if (object->itemAt(i)->layout()) {
//    //            setChildrensFont(object->layout()->itemAt(i)->layout(), font, indent + 2);
//    //        }

//    //        if (object->itemAt(i)->widget()) {
//    //            object->itemAt(i)->widget()->setFont(font);

//    //            if (object->itemAt(i)->widget()->layout() &&
//    //                !object->itemAt(i)->widget()->layout()->isEmpty()) {
//    //                setChildrensFont(object->itemAt(i)->widget()->layout(), font, indent +
//    2);
//    //            }
//    //        }
//    //    }
//}

///// Widget creation helpers
void SettingsDialog::okButtonClicked()
{
    this->close();
}

void SettingsDialog::cancelButtonClicked()
{
    for (auto &tab : this->tabs) {
        tab->getSettingsPage()->cancel();
    }

    getApp()->settings->recallSnapshot();

    this->close();
}

}  // namespace chatterino
