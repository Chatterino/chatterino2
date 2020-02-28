#include "widgets/dialogs/SettingsDialog.hpp"

#include "Application.hpp"
#include "singletons/Resources.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/Button.hpp"
#include "widgets/settingspages/AboutPage.hpp"
#include "widgets/settingspages/AccountsPage.hpp"
#include "widgets/settingspages/CommandPage.hpp"
#include "widgets/settingspages/ExternalToolsPage.hpp"
#include "widgets/settingspages/GeneralPage.hpp"
#include "widgets/settingspages/HighlightingPage.hpp"
#include "widgets/settingspages/IgnoresPage.hpp"
#include "widgets/settingspages/KeyboardSettingsPage.hpp"
#include "widgets/settingspages/ModerationPage.hpp"
#include "widgets/settingspages/NotificationPage.hpp"

#include <QDialogButtonBox>
#include <QLineEdit>

namespace chatterino {

SettingsDialog *SettingsDialog::instance_ = nullptr;

SettingsDialog::SettingsDialog()
    : BaseWindow(BaseWindow::DisableCustomScaling)
{
    this->setWindowTitle("Chatterino Settings");
    this->resize(815, 600);
    this->themeChangedEvent();
    this->scaleChangedEvent(this->scale());

    this->initUi();
    this->addTabs();
    this->overrideBackgroundColor_ = QColor("#111111");
    this->scaleChangedEvent(this->scale());  // execute twice to width of item
}

void SettingsDialog::initUi()
{
    auto outerBox = LayoutCreator<QWidget>(this->getLayoutContainer())
                        .setLayoutType<QVBoxLayout>()
                        .withoutSpacing();

    // TOP
    auto title = outerBox.emplace<PageHeader>();
    auto edit = LayoutCreator<PageHeader>(title.getElement())
                    .setLayoutType<QHBoxLayout>()
                    .withoutMargin()
                    .emplace<QLineEdit>()
                    .assign(&this->ui_.search);
    edit->setPlaceholderText("Find in settings...");

    QObject::connect(edit.getElement(), &QLineEdit::textChanged, this,
                     &SettingsDialog::filterElements);

    // CENTER
    auto centerBox =
        outerBox.emplace<QHBoxLayout>().withoutMargin().withoutSpacing();

    // left side (tabs)
    centerBox.emplace<QWidget>()
        .assign(&this->ui_.tabContainerContainer)
        .setLayoutType<QVBoxLayout>()
        .withoutMargin()
        .assign(&this->ui_.tabContainer);

    // right side (pages)
    centerBox.emplace<QStackedLayout>()
        .assign(&this->ui_.pageStack)
        .withoutMargin();

    this->ui_.pageStack->setMargin(0);

    outerBox->addSpacing(12);

    // BOTTOM
    auto buttons = outerBox.emplace<QDialogButtonBox>(Qt::Horizontal);
    {
        this->ui_.okButton =
            buttons->addButton("Ok", QDialogButtonBox::YesRole);
        this->ui_.cancelButton =
            buttons->addButton("Cancel", QDialogButtonBox::NoRole);
    }

    // ---- misc
    this->ui_.tabContainerContainer->setObjectName("tabWidget");
    this->ui_.pageStack->setObjectName("pages");

    QObject::connect(this->ui_.okButton, &QPushButton::clicked, this,
                     &SettingsDialog::onOkClicked);
    QObject::connect(this->ui_.cancelButton, &QPushButton::clicked, this,
                     &SettingsDialog::onCancelClicked);
}

void SettingsDialog::filterElements(const QString &text)
{
    // filter elements and hide pages
    for (auto &&tab : this->tabs_)
    {
        // filterElements returns true if anything on the page matches the search query
        tab->setVisible(tab->page()->filterElements(text) ||
                        tab->name().contains(text, Qt::CaseInsensitive));
    }

    // find next visible page
    if (this->lastSelectedByUser_ && this->lastSelectedByUser_->isVisible())
    {
        this->selectTab(this->lastSelectedByUser_, false);
    }
    else if (!this->selectedTab_->isVisible())
    {
        for (auto &&tab : this->tabs_)
        {
            if (tab->isVisible())
            {
                this->selectTab(tab, false);
                break;
            }
        }
    }

    // remove duplicate spaces
    bool shouldShowSpace = false;

    for (int i = 0; i < this->ui_.tabContainer->count(); i++)
    {
        auto item = this->ui_.tabContainer->itemAt(i);
        if (auto x = dynamic_cast<QSpacerItem *>(item); x)
        {
            x->changeSize(10, shouldShowSpace ? int(16 * this->scale()) : 0);
            shouldShowSpace = false;
        }
        else if (item->widget())
        {
            shouldShowSpace |= item->widget()->isVisible();
        }
    }
}

SettingsDialog *SettingsDialog::instance()
{
    return SettingsDialog::instance_;
}

void SettingsDialog::addTabs()
{
    this->ui_.tabContainer->setMargin(0);
    this->ui_.tabContainer->setSpacing(0);

    this->ui_.tabContainer->setContentsMargins(0, 20, 0, 20);

    // Constructors are wrapped in std::function to remove some strain from first time loading.

    // clang-format off
    this->addTab([]{return new GeneralPage;},          "General",        ":/settings/about.svg");
    this->ui_.tabContainer->addSpacing(16);
    this->addTab([]{return new AccountsPage;},         "Accounts",       ":/settings/accounts.svg", SettingsTabId::Accounts);
    this->ui_.tabContainer->addSpacing(16);
    this->addTab([]{return new CommandPage;},          "Commands",       ":/settings/commands.svg");
    this->addTab([]{return new HighlightingPage;},     "Highlights",     ":/settings/notifications.svg");
    this->addTab([]{return new IgnoresPage;},          "Ignores",        ":/settings/ignore.svg");
    this->ui_.tabContainer->addSpacing(16);
    this->addTab([]{return new KeyboardSettingsPage;}, "Keybindings",    ":/settings/keybinds.svg");
    this->addTab([]{return new ModerationPage;},       "Moderation",     ":/settings/moderation.svg", SettingsTabId::Moderation);
    this->addTab([]{return new NotificationPage;},     "Notifications",  ":/settings/notification2.svg");
    this->addTab([]{return new ExternalToolsPage;},    "External tools", ":/settings/externaltools.svg");
    this->ui_.tabContainer->addStretch(1);
    this->addTab([]{return new AboutPage;},            "About",          ":/settings/about.svg", SettingsTabId(), Qt::AlignBottom);
    // clang-format on
}

void SettingsDialog::addTab(std::function<SettingsPage *()> page,
                            const QString &name, const QString &iconPath,
                            SettingsTabId id, Qt::Alignment alignment)
{
    auto tab = new SettingsDialogTab(this, std::move(page), name, iconPath, id);

    this->ui_.tabContainer->addWidget(tab, 0, alignment);
    this->tabs_.push_back(tab);

    if (this->tabs_.size() == 1)
    {
        this->selectTab(tab);
    }
}

void SettingsDialog::selectTab(SettingsDialogTab *tab, bool byUser)
{
    // add page if it's not been added yet
    [&] {
        for (int i = 0; i < this->ui_.pageStack->count(); i++)
            if (this->ui_.pageStack->itemAt(i)->widget() == tab->page())
                return;

        this->ui_.pageStack->addWidget(tab->page());
    }();

    this->ui_.pageStack->setCurrentWidget(tab->page());

    if (this->selectedTab_ != nullptr)
    {
        this->selectedTab_->setSelected(false);
        this->selectedTab_->setStyleSheet("color: #FFF");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #222; color: #4FC3F7;"
                       "/*border: 1px solid #555; border-right: none;*/");
    this->selectedTab_ = tab;
    if (byUser)
    {
        this->lastSelectedByUser_ = tab;
    }
}

void SettingsDialog::selectTab(SettingsTabId id)
{
    auto t = this->tab(id);
    assert(t);
    if (!t)
        return;

    this->selectTab(t);
}

SettingsDialogTab *SettingsDialog::tab(SettingsTabId id)
{
    for (auto &&tab : this->tabs_)
        if (tab->id() == id)
            return tab;

    assert(false);
    return nullptr;
}

void SettingsDialog::showDialog(SettingsDialogPreference preferredTab)
{
    static SettingsDialog *instance = new SettingsDialog();
    static bool hasShownBefore = false;
    if (hasShownBefore)
        instance->refresh();
    hasShownBefore = true;

    switch (preferredTab)
    {
        case SettingsDialogPreference::Accounts:
            instance->selectTab(SettingsTabId::Accounts);
            break;

        case SettingsDialogPreference::ModerationActions:
            if (auto tab = instance->tab(SettingsTabId::Moderation))
            {
                instance->selectTab(tab);
                if (auto page = dynamic_cast<ModerationPage *>(tab->page()))
                {
                    page->selectModerationActions();
                }
            }
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
    // Resets the cancel button.
    getSettings()->saveSnapshot();

    // Updates tabs.
    for (auto *tab : this->tabs_)
    {
        tab->page()->onShow();
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

    if (this->ui_.tabContainerContainer)
        this->ui_.tabContainerContainer->setFixedWidth(int(150 * newDpi));
}

void SettingsDialog::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#111"));
    this->setPalette(palette);
}

void SettingsDialog::showEvent(QShowEvent *)
{
    this->ui_.search->setText("");
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
        tab->page()->cancel();
    }

    getSettings()->restoreSnapshot();

    this->close();
}

}  // namespace chatterino
