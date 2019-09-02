#include "widgets/dialogs/SettingsDialog.hpp"

#include "Application.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/SettingsDialogTab.hpp"
#include "widgets/settingspages/AboutPage.hpp"
#include "widgets/settingspages/AccountsPage.hpp"
#include "widgets/settingspages/AdvancedPage.hpp"
#include "widgets/settingspages/BrowserExtensionPage.hpp"
#include "widgets/settingspages/CommandPage.hpp"
#include "widgets/settingspages/EmotesPage.hpp"
#include "widgets/settingspages/ExternalToolsPage.hpp"
#include "widgets/settingspages/FeelPage.hpp"
#include "widgets/settingspages/GeneralPage.hpp"
#include "widgets/settingspages/HighlightingPage.hpp"
#include "widgets/settingspages/IgnoresPage.hpp"
#include "widgets/settingspages/KeyboardSettingsPage.hpp"
#include "widgets/settingspages/LogsPage.hpp"
#include "widgets/settingspages/LookPage.hpp"
#include "widgets/settingspages/ModerationPage.hpp"
#include "widgets/settingspages/NotificationPage.hpp"
#include "widgets/settingspages/SpecialChannelsPage.hpp"

#include <QDialogButtonBox>
#include <QLineEdit>

namespace chatterino {

SettingsDialog *SettingsDialog::handle = nullptr;

SettingsDialog::SettingsDialog()
    : BaseWindow(nullptr, BaseWindow::DisableCustomScaling)
{
    this->initUi();
    this->addTabs();

    this->scaleChangedEvent(this->scale());

    this->overrideBackgroundColor_ = QColor("#111111");
    this->themeChangedEvent();

    this->resize(815, 600);
}

void SettingsDialog::initUi()
{
    LayoutCreator<SettingsDialog> layoutCreator(this);

    // tab pages
    layoutCreator.setLayoutType<QHBoxLayout>()
        .withoutSpacing()
        .emplace<QWidget>()
        .assign(&this->ui_.tabContainerContainer)
        .emplace<QVBoxLayout>()
        .withoutMargin()
        .assign(&this->ui_.tabContainer);

    this->ui_.tabContainerContainer->layout()->setContentsMargins(8, 8, 0, 39);

    this->layout()->setSpacing(0);

    // right side layout
    auto right = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        auto title = right.emplace<QHBoxLayout>().withoutMargin();

        auto edit = title.emplace<QLineEdit>().assign(&this->ui_.search);
        QTimer::singleShot(100, edit.getElement(),
                           [edit = edit.getElement()]() { edit->setFocus(); });
        QObject::connect(
            edit.getElement(), &QLineEdit::textChanged, this,
            [this](const QString &text) {
                // filter elements and hide pages
                for (auto &&page : this->pages_)
                {
                    // filterElements returns true if anything on the page matches the search query
                    page->tab()->setVisible(page->filterElements(text));
                }

                // TODO: add originally selected page

                // find next visible page
                if (!this->selectedTab_->isVisible())
                {
                    for (auto &&tab : this->tabs_)
                    {
                        if (tab->isVisible())
                        {
                            this->selectTab(tab);
                            break;
                        }
                    }
                }

                // remove duplicate spaces
                bool shouldShowSpace = true;

                for (int i = 0; i < this->ui_.tabContainer->count(); i++)
                {
                    auto item = this->ui_.tabContainer->itemAt(i);
                    if (auto x = dynamic_cast<QSpacerItem *>(item); x)
                    {
                        x->changeSize(
                            10, shouldShowSpace ? int(16 * this->scale()) : 0);
                        shouldShowSpace = false;
                    }
                    else if (item->widget())
                    {
                        shouldShowSpace |= item->widget()->isVisible();
                    }
                }
            });

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

    this->ui_.pageStack->setMargin(0);

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
    this->ui_.tabContainer->setMargin(0);
    this->ui_.tabContainer->setSpacing(0);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new GeneralPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new AccountsPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new CommandPage);
    this->addTab(new HighlightingPage);
    this->addTab(new IgnoresPage);

    this->ui_.tabContainer->addSpacing(16);

    this->addTab(new KeyboardSettingsPage);
    this->addTab(this->ui_.moderationPage = new ModerationPage);
    this->addTab(new NotificationPage);
    this->addTab(new ExternalToolsPage);
    this->addTab(new AdvancedPage);

    this->ui_.tabContainer->addStretch(1);
    this->addTab(new AboutPage, Qt::AlignBottom);
    this->ui_.tabContainer->addSpacing(16);
}

void SettingsDialog::addTab(SettingsPage *page, Qt::Alignment alignment)
{
    auto tab = new SettingsDialogTab(this, page, page->getIconResource());
    page->setTab(tab);

    this->ui_.pageStack->addWidget(page);
    this->ui_.tabContainer->addWidget(tab, 0, alignment);
    this->tabs_.push_back(tab);
    this->pages_.push_back(page);

    if (this->tabs_.size() == 1)
    {
        this->selectTab(tab);
    }
}

void SettingsDialog::selectTab(SettingsDialogTab *tab)
{
    this->ui_.pageStack->setCurrentWidget(tab->getSettingsPage());

    if (this->selectedTab_ != nullptr)
    {
        this->selectedTab_->setSelected(false);
        this->selectedTab_->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #222; color: #4FC3F7;"
                       "border-left: 1px solid #444;"
                       "border-top: 1px solid #444;"
                       "border-bottom: 1px solid #444;");
    this->selectedTab_ = tab;
}

void SettingsDialog::selectPage(SettingsPage *page)
{
    assert(page);
    assert(page->tab());

    this->selectTab(page->tab());
}

void SettingsDialog::showDialog(SettingsDialogPreference preferredTab)
{
    static SettingsDialog *instance = new SettingsDialog();
    instance->refresh();

    switch (preferredTab)
    {
        case SettingsDialogPreference::Accounts:
            instance->selectTab(instance->tabs_.at(1));
            break;

        case SettingsDialogPreference::ModerationActions:
            instance->selectPage(instance->ui_.moderationPage);
            instance->ui_.moderationPage->selectModerationActions();
            break;

        default:;
    }

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void SettingsDialog::refresh()
{
    getSettings()->saveSnapshot();

    for (auto *tab : this->tabs_)
    {
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

    for (SettingsDialogTab *tab : this->tabs_)
    {
        tab->setFixedHeight(int(30 * newDpi));
    }

    this->setStyleSheet(styleSheet);

    this->ui_.tabContainerContainer->setFixedWidth(int(150 * newDpi));
}

void SettingsDialog::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#111"));
    this->setPalette(palette);
}

///// Widget creation helpers
void SettingsDialog::onOkClicked()
{
    pajlada::Settings::SettingManager::gSave();
    this->close();
}

void SettingsDialog::onCancelClicked()
{
    for (auto &tab : this->tabs_)
    {
        tab->getSettingsPage()->cancel();
    }

    getSettings()->restoreSnapshot();

    this->close();
}

}  // namespace chatterino
