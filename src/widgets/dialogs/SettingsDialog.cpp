#include "widgets/dialogs/SettingsDialog.hpp"

#include "Application.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/SettingsDialogTab.hpp"
#include "widgets/settingspages/AboutPage.hpp"
#include "widgets/settingspages/AccountsPage.hpp"
#include "widgets/settingspages/BrowserExtensionPage.hpp"
#include "widgets/settingspages/CommandPage.hpp"
#include "widgets/settingspages/EmotesPage.hpp"
#include "widgets/settingspages/ExternalToolsPage.hpp"
#include "widgets/settingspages/FeelPage.hpp"
#include "widgets/settingspages/HighlightingPage.hpp"
#include "widgets/settingspages/IgnoresPage.hpp"
#include "widgets/settingspages/KeyboardSettingsPage.hpp"
#include "widgets/settingspages/LogsPage.hpp"
#include "widgets/settingspages/LookPage.hpp"
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

    this->overrideBackgroundColor_ = QColor("#282828");
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
        right.emplace<QStackedLayout>()
            .assign(&this->ui_.pageStack)
            .withoutMargin();

        auto buttons = right.emplace<QDialogButtonBox>(Qt::Horizontal);
        {
            this->ui_.okButton =
                buttons->addButton("Ok", QDialogButtonBox::YesRole);
            this->ui_.cancelButton =
                buttons->addButton("Cancel", QDialogButtonBox::NoRole);
        }
    }

    // ---- misc
    this->ui_.tabContainerContainer->setObjectName("tabWidget");
    this->ui_.pageStack->setObjectName("pages");

    QObject::connect(this->ui_.okButton, &QPushButton::clicked, this,
                     &SettingsDialog::onOkClicked);
    QObject::connect(this->ui_.cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::onCancelClicked);
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

    this->addTab(new LookPage);
    this->addTab(new FeelPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new CommandPage);
    //    this->addTab(new EmotesPage);
    this->addTab(new HighlightingPage);
    this->addTab(new IgnoresPage);

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
    this->tabs_.push_back(tab);

    if (this->tabs_.size() == 1) {
        this->selectTab(tab);
    }
}

void SettingsDialog::selectTab(SettingsDialogTab *tab)
{
    this->ui_.pageStack->setCurrentWidget(tab->getSettingsPage());

    if (this->selectedTab_ != nullptr) {
        this->selectedTab_->setSelected(false);
        this->selectedTab_->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #555; color: #FFF");
    this->selectedTab_ = tab;
}

void SettingsDialog::showDialog(PreferredTab preferredTab)
{
    static SettingsDialog *instance = new SettingsDialog();
    instance->refresh();

    switch (preferredTab) {
        case SettingsDialog::PreferredTab::Accounts: {
            instance->selectTab(instance->tabs_.at(0));
        } break;
    }

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void SettingsDialog::refresh()
{
    getSettings()->saveSnapshot();

    for (auto *tab : this->tabs_) {
        tab->getSettingsPage()->onShow();
    }
}

void SettingsDialog::scaleChangedEvent(float newDpi)
{
    QFile file(":/qss/settings.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    styleSheet.replace("<font-size>", QString::number(int(14 * newDpi)));
    styleSheet.replace("<checkbox-size>", QString::number(int(14 * newDpi)));

    for (SettingsDialogTab *tab : this->tabs_) {
        tab->setFixedHeight(int(30 * newDpi));
    }

    this->setStyleSheet(styleSheet);

    this->ui_.tabContainerContainer->setFixedWidth(int(200 * newDpi));
}

void SettingsDialog::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    this->setPalette(palette);
}

///// Widget creation helpers
void SettingsDialog::onOkClicked()
{
    this->close();
}

void SettingsDialog::onCancelClicked()
{
    for (auto &tab : this->tabs_) {
        tab->getSettingsPage()->cancel();
    }

    getSettings()->restoreSnapshot();

    this->close();
}

}  // namespace chatterino
