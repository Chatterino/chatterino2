#include "widgets/settingsdialog.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/helper/settingsdialogtab.hpp"
#include "widgets/settingspages/aboutpage.hpp"
#include "widgets/settingspages/accountspage.hpp"
#include "widgets/settingspages/appearancepage.hpp"
#include "widgets/settingspages/behaviourpage.hpp"
#include "widgets/settingspages/commandpage.hpp"
#include "widgets/settingspages/emotespage.hpp"
#include "widgets/settingspages/highlightingpage.hpp"
#include "widgets/settingspages/ignoremessagespage.hpp"
#include "widgets/settingspages/ignoreuserspage.hpp"
#include "widgets/settingspages/logspage.hpp"
#include "widgets/settingspages/moderationpage.hpp"

#include <QDialogButtonBox>

namespace chatterino {
namespace widgets {

SettingsDialog *SettingsDialog::handle = nullptr;

SettingsDialog::SettingsDialog()
    : BaseWindow()
{
    this->initUi();

    this->addTabs();

    this->dpiMultiplierChanged(this->getDpiMultiplier(), this->getDpiMultiplier());
}

void SettingsDialog::initUi()
{
    util::LayoutCreator<SettingsDialog> layoutCreator(this);

    // tab pages
    layoutCreator.emplace<QWidget>()
        .assign(&this->ui.tabContainerContainer)
        .emplace<QVBoxLayout>()
        .withoutMargin()
        .assign(&this->ui.tabContainer);

    // right side layout
    auto right = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        right.emplace<QStackedLayout>().assign(&this->ui.pageStack).withoutMargin();

        auto buttons = right.emplace<QDialogButtonBox>(Qt::Horizontal);
        {
            this->ui.okButton = buttons->addButton("Ok", QDialogButtonBox::YesRole);
            this->ui.cancelButton = buttons->addButton("Cancel", QDialogButtonBox::NoRole);
        }
    }

    // ---- misc
    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    this->setPalette(palette);

    this->ui.tabContainerContainer->setObjectName("tabWidget");
    this->ui.pageStack->setObjectName("pages");

    QObject::connect(this->ui.okButton, &QPushButton::clicked, this,
                     &SettingsDialog::okButtonClicked);
    QObject::connect(this->ui.cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::cancelButtonClicked);
}

SettingsDialog *SettingsDialog::getHandle()
{
    return SettingsDialog::handle;
}

void SettingsDialog::addTabs()
{
    this->addTab(new settingspages::AccountsPage);
    this->addTab(new settingspages::AppearancePage);
    this->addTab(new settingspages::BehaviourPage);
    this->addTab(new settingspages::CommandPage);
    this->addTab(new settingspages::EmotesPage);
    this->addTab(new settingspages::IgnoreUsersPage);
    this->addTab(new settingspages::IgnoreMessagesPage);
    this->addTab(new settingspages::HighlightingPage);
    //    this->addTab(new settingspages::LogsPage);
    this->addTab(new settingspages::ModerationPage);
    this->ui.tabContainer->addStretch(1);
    this->addTab(new settingspages::AboutPage, Qt::AlignBottom);
}

void SettingsDialog::addTab(settingspages::SettingsPage *page, Qt::Alignment alignment)
{
    auto tab = new SettingsDialogTab(this, page, page->getIconResource());

    this->ui.pageStack->addWidget(page);
    this->ui.tabContainer->addWidget(tab, 0, alignment);
    this->tabs.push_back(tab);

    if (this->tabs.size() == 1) {
        this->select(tab);
    }
}

void SettingsDialog::select(SettingsDialogTab *tab)
{
    this->ui.pageStack->setCurrentWidget(tab->getSettingsPage());

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

    singletons::SettingManager::getInstance().saveSnapshot();
}

void SettingsDialog::dpiMultiplierChanged(float oldDpi, float newDpi)
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

    this->ui.tabContainerContainer->setFixedWidth((int)(200 * newDpi));
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
    auto &settings = singletons::SettingManager::getInstance();

    for (auto &tab : this->tabs) {
        tab->getSettingsPage()->cancel();
    }

    settings.recallSnapshot();

    this->close();
}

}  // namespace widgets
}  // namespace chatterino
