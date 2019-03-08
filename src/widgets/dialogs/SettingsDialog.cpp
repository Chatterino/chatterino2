#include "widgets/dialogs/SettingsDialog.hpp"

#include "Application.hpp"
#include "ab/Column.hpp"
#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"
#include "widgets/helper/SettingsDialogTab.hpp"
#include "widgets/settingspages/AboutPage.hpp"
#include "widgets/settingspages/AccountsPage.hpp"
#include "widgets/settingspages/AdvancedPage.hpp"
#include "widgets/settingspages/CommandPage.hpp"
#include "widgets/settingspages/ExternalToolsPage.hpp"
#include "widgets/settingspages/GeneralPage.hpp"
#include "widgets/settingspages/HighlightingPage.hpp"
#include "widgets/settingspages/IgnoresPage.hpp"
#include "widgets/settingspages/KeyboardSettingsPage.hpp"
#include "widgets/settingspages/ModerationPage.hpp"
#include "widgets/settingspages/NotificationPage.hpp"

#include <QDialogButtonBox>
#include <QStyle>

namespace chatterino
{
    SettingsDialog::SettingsDialog()
        : ab::BaseWindow(BaseWindow::DisableCustomScaling)
    {
        this->initializeLayout();
        this->addTabs();

        this->resize(766, 600);

        QFile qss(":/style/settings.qss");
        qss.open(QIODevice::ReadOnly);
        this->setScalableQss(qss.readAll());
    }

    void SettingsDialog::initializeLayout()
    {
        this->setCenterLayout(ab::makeLayout<ab::Row>({
            // left side
            ab::makeWidget<QWidget>([&](auto w) {
                this->ui_.tabContainerContainer = w;
                w->setObjectName("tabWidget");
                w->setLayout(ab::makeLayout<ab::Column>(
                    [&](auto w) { this->ui_.tabContainer = w; }, {}));
            }),

            // right side
            ab::makeLayout<ab::Column>({
                // pages
                ab::makeWidget<QStackedLayout>([&](auto w) {
                    this->ui_.pageStack = w;
                    this->ui_.pageStack->setObjectName("pages");
                }),

                // buttons
                ab::makeWidget<QDialogButtonBox>([&](auto w) {
                    this->ui_.okButton =
                        w->addButton("Ok", QDialogButtonBox::YesRole);
                    this->ui_.cancelButton =
                        w->addButton("Cancel", QDialogButtonBox::NoRole);

                    QObject::connect(this->ui_.okButton, &QPushButton::clicked,
                        this, &SettingsDialog::onOkClicked);
                    QObject::connect(this->ui_.cancelButton,
                        &QPushButton::clicked, this,
                        &SettingsDialog::onCancelClicked);
                }),
            }),
        }));
    }

    SettingsDialog* SettingsDialog::getHandle()
    {
        return SettingsDialog::instance;
    }

    void SettingsDialog::addTabs()
    {
        // TODO: scale the spacing

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
    }

    void SettingsDialog::addTab(SettingsPage* page, Qt::Alignment alignment)
    {
        auto tab = new SettingsDialogTab(this, page, page->getIconResource());
        page->setTab(tab);

        this->ui_.pageStack->addWidget(page);
        this->ui_.tabContainer->addWidget(tab, 0, alignment);
        this->tabs_.push_back(tab);

        if (this->tabs_.size() == 1)
        {
            this->selectTab(tab);
        }
    }

    void SettingsDialog::selectTab(SettingsDialogTab* tab)
    {
        this->ui_.pageStack->setCurrentWidget(tab->getSettingsPage());

        if (this->selectedTab_ != nullptr)
        {
            this->selectedTab_->setProperty("selected", {});
            this->selectedTab_->style()->unpolish(this->selectedTab_);
            this->selectedTab_->style()->polish(this->selectedTab_);
            this->selectedTab_->setSelected(false);
        }

        tab->setProperty("selected", "true");
        tab->style()->unpolish(tab);
        tab->style()->polish(tab);
        tab->setSelected(true);

        this->selectedTab_ = tab;
    }

    void SettingsDialog::selectPage(SettingsPage* page)
    {
        assert(page);
        assert(page->tab());

        this->selectTab(page->tab());
    }

    void SettingsDialog::showDialog(SettingsDialogPreference preferredTab)
    {
        static SettingsDialog* instance = new SettingsDialog();
        instance->refresh();

        switch (preferredTab)
        {
            case SettingsDialogPreference::Accounts:
                instance->selectTab(instance->tabs_.at(0));
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

        for (auto* tab : this->tabs_)
        {
            tab->getSettingsPage()->onShow();
        }
    }

    ///// Widget creation helpers
    void SettingsDialog::onOkClicked()
    {
        pajlada::Settings::SettingManager::gSave();
        this->close();
    }

    void SettingsDialog::onCancelClicked()
    {
        for (auto& tab : this->tabs_)
        {
            tab->getSettingsPage()->cancel();
        }

        getSettings()->restoreSnapshot();

        this->close();
    }
}  // namespace chatterino
