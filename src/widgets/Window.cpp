#include "widgets/Window.hpp"

#include "Application.hpp"
#include "common/Credentials.hpp"
#include "common/Modes.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Updates.hpp"
#include "singletons/WindowManager.hpp"
#include "util/InitUpdateButton.hpp"
#include "widgets/AccountSwitchPopup.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/dialogs/switcher/QuickSwitcherPopup.hpp"
#include "widgets/dialogs/UpdateDialog.hpp"
#include "widgets/dialogs/WelcomeDialog.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/TitlebarButton.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/ClosedSplits.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#ifndef NDEBUG
#    include "providers/twitch/PubSubManager.hpp"
#    include "providers/twitch/PubSubMessages.hpp"
#    include "util/SampleData.hpp"

#    include <rapidjson/document.h>
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QHeaderView>
#include <QMenuBar>
#include <QPalette>
#include <QStandardItemModel>
#include <QVBoxLayout>

namespace chatterino {

Window::Window(WindowType type, QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
    , type_(type)
    , notebook_(new SplitNotebook(this))
{
    this->addCustomTitlebarButtons();
    this->addShortcuts();
    this->addLayout();

#ifdef Q_OS_MACOS
    this->addMenuBar();
#endif

    this->bSignals_.emplace_back(
        getApp()->accounts->twitch.currentUserChanged.connect([this] {
            this->onAccountSelected();
        }));
    this->onAccountSelected();

    if (type == WindowType::Main)
    {
        this->resize(int(600 * this->scale()), int(500 * this->scale()));
    }
    else
    {
        this->resize(int(300 * this->scale()), int(500 * this->scale()));
    }

    this->signalHolder_.managedConnect(getApp()->hotkeys->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });
    if (type == WindowType::Main || type == WindowType::Popup)
    {
        getSettings()->tabDirection.connect(
            [this](int val) {
                this->notebook_->setTabLocation(NotebookTabLocation(val));
            },
            this->signalHolder_);
    }
}

WindowType Window::getType()
{
    return this->type_;
}

SplitNotebook &Window::getNotebook()
{
    return *this->notebook_;
}

bool Window::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::WindowActivate: {
            getApp()->windows->selectedWindow_ = this;
            break;
        }

        case QEvent::WindowDeactivate: {
            auto *page = this->notebook_->getSelectedPage();

            if (page != nullptr)
            {
                std::vector<Split *> splits = page->getSplits();

                for (Split *split : splits)
                {
                    split->updateLastReadMessage();
                }

                page->hideResizeHandles();
            }
        }
        break;

        default:;
    }

    return BaseWindow::event(event);
}

void Window::closeEvent(QCloseEvent *)
{
    if (this->type_ == WindowType::Main)
    {
        auto app = getApp();
        app->windows->save();
        app->windows->closeAll();
    }

    // Ensure selectedWindow_ is never an invalid pointer.
    // WindowManager will return the main window if no window is pointed to by
    // `selectedWindow_`.
    getApp()->windows->selectedWindow_ = nullptr;

    this->closed.invoke();

    if (this->type_ == WindowType::Main)
    {
        QApplication::exit();
    }
}

void Window::addLayout()
{
    auto *layout = new QVBoxLayout();

    layout->addWidget(this->notebook_);
    this->getLayoutContainer()->setLayout(layout);

    // set margin
    layout->setContentsMargins(0, 0, 0, 0);

    this->notebook_->setAllowUserTabManagement(true);
    this->notebook_->setShowAddButton(true);
}

void Window::addCustomTitlebarButtons()
{
    if (!this->hasCustomWindowFrame())
        return;
    if (this->type_ != WindowType::Main)
        return;

    // settings
    this->addTitleBarButton(TitleBarButtonStyle::Settings, [this] {
        getApp()->windows->showSettingsDialog(this);
    });

    // updates
    auto update = this->addTitleBarButton(TitleBarButtonStyle::None, [] {});

    initUpdateButton(*update, this->signalHolder_);

    // account
    this->userLabel_ = this->addTitleBarLabel([this] {
        getApp()->windows->showAccountSelectPopup(this->userLabel_->mapToGlobal(
            this->userLabel_->rect().bottomLeft()));
    });
    this->userLabel_->setMinimumWidth(20 * scale());

    // streamer mode
    this->streamerModeTitlebarIcon_ =
        this->addTitleBarButton(TitleBarButtonStyle::StreamerMode, [this] {
            getApp()->windows->showSettingsDialog(
                this, SettingsDialogPreference::StreamerMode);
        });
    this->signalHolder_.managedConnect(getApp()->streamerModeChanged, [this]() {
        this->updateStreamerModeIcon();
    });

    // Update initial state
    this->updateStreamerModeIcon();
}

void Window::updateStreamerModeIcon()
{
    // A duplicate of this code is in SplitNotebook class (in Notebook.{c,h}pp)
    // That one is the one near splits (on linux and mac or non-main windows on Windows)
    // This copy handles the TitleBar icon in Window (main window on Windows)
    if (this->streamerModeTitlebarIcon_ == nullptr)
    {
        return;
    }
#ifdef Q_OS_WIN
    assert(this->getType() == WindowType::Main);
    if (getTheme()->isLightTheme())
    {
        this->streamerModeTitlebarIcon_->setPixmap(
            getResources().buttons.streamerModeEnabledLight);
    }
    else
    {
        this->streamerModeTitlebarIcon_->setPixmap(
            getResources().buttons.streamerModeEnabledDark);
    }
    this->streamerModeTitlebarIcon_->setVisible(isInStreamerMode());
#else
    // clang-format off
    assert(false && "Streamer mode TitleBar icon should not exist on non-Windows OSes");
    // clang-format on
#endif
}

void Window::themeChangedEvent()
{
    this->updateStreamerModeIcon();
    BaseWindow::themeChangedEvent();
}

void Window::addDebugStuff(HotkeyController::HotkeyMap &actions)
{
#ifndef NDEBUG
    actions.emplace("addMiscMessage", [=](std::vector<QString>) -> QString {
        const auto &messages = getSampleMiscMessages();
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch->addFakeMessage(msg);
        return "";
    });

    actions.emplace("addCheerMessage", [=](std::vector<QString>) -> QString {
        const auto &messages = getSampleCheerMessages();
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch->addFakeMessage(msg);
        return "";
    });

    actions.emplace("addLinkMessage", [=](std::vector<QString>) -> QString {
        const auto &messages = getSampleLinkMessages();
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch->addFakeMessage(msg);
        return "";
    });

    actions.emplace("addRewardMessage", [=](std::vector<QString>) -> QString {
        rapidjson::Document doc;
        auto app = getApp();
        static bool alt = true;
        if (alt)
        {
            auto oMessage =
                parsePubSubBaseMessage(getSampleChannelRewardMessage());
            auto oInnerMessage =
                oMessage->toInner<PubSubMessageMessage>()
                    ->toInner<PubSubCommunityPointsChannelV1Message>();

            app->twitch->addFakeMessage(getSampleChannelRewardIRCMessage());
            app->twitch->pubsub->signals_.pointReward.redeemed.invoke(
                oInnerMessage->data.value("redemption").toObject());
            alt = !alt;
        }
        else
        {
            auto oMessage =
                parsePubSubBaseMessage(getSampleChannelRewardMessage2());
            auto oInnerMessage =
                oMessage->toInner<PubSubMessageMessage>()
                    ->toInner<PubSubCommunityPointsChannelV1Message>();
            app->twitch->pubsub->signals_.pointReward.redeemed.invoke(
                oInnerMessage->data.value("redemption").toObject());
            alt = !alt;
        }
        return "";
    });

    actions.emplace("addEmoteMessage", [=](std::vector<QString>) -> QString {
        const auto &messages = getSampleEmoteTestMessages();
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch->addFakeMessage(msg);
        return "";
    });

    actions.emplace("addSubMessage", [=](std::vector<QString>) -> QString {
        const auto &messages = getSampleSubMessages();
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch->addFakeMessage(msg);
        return "";
    });
#endif
}

void Window::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"openSettings",  // Open settings
         [this](std::vector<QString>) -> QString {
             SettingsDialog::showDialog(this);
             return "";
         }},
        {"newSplit",  // Create a new split
         [this](std::vector<QString>) -> QString {
             this->notebook_->getOrAddSelectedPage()->appendNewSplit(true);
             return "";
         }},
        {"openTab",  // CTRL + 1-8 to open corresponding tab.
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "openTab shortcut called without arguments. "
                        "Takes only "
                        "one argument: tab specifier";
                 return "openTab shortcut called without arguments. "
                        "Takes only "
                        "one argument: tab specifier";
             }
             auto target = arguments.at(0);
             if (target == "last")
             {
                 this->notebook_->selectLastTab();
             }
             else if (target == "next")
             {
                 this->notebook_->selectNextTab();
             }
             else if (target == "previous")
             {
                 this->notebook_->selectPreviousTab();
             }
             else
             {
                 bool ok;
                 int result = target.toInt(&ok);
                 if (ok)
                 {
                     this->notebook_->selectVisibleIndex(result);
                 }
                 else
                 {
                     qCWarning(chatterinoHotkeys)
                         << "Invalid argument for openTab shortcut";
                     return QString("Invalid argument for openTab "
                                    "shortcut: \"%1\". Use \"last\", "
                                    "\"next\", \"previous\" or an integer.")
                         .arg(target);
                 }
             }
             return "";
         }},
        {"popup",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 return "popup action called without arguments. Takes only "
                        "one: \"split\" or \"window\".";
             }
             if (arguments.at(0) == "split")
             {
                 if (auto page = dynamic_cast<SplitContainer *>(
                         this->notebook_->getSelectedPage()))
                 {
                     if (auto split = page->getSelectedSplit())
                     {
                         split->popup();
                     }
                 }
             }
             else if (arguments.at(0) == "window")
             {
                 if (auto page = dynamic_cast<SplitContainer *>(
                         this->notebook_->getSelectedPage()))
                 {
                     page->popup();
                 }
             }
             else
             {
                 return "Invalid popup target. Use \"split\" or \"window\".";
             }
             return "";
         }},
        {"zoom",
         [](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "zoom shortcut called without arguments. Takes "
                        "only "
                        "one argument: \"in\", \"out\", or \"reset\"";
                 return "zoom shortcut called without arguments. Takes "
                        "only "
                        "one argument: \"in\", \"out\", or \"reset\"";
             }
             auto change = 0.0f;
             auto direction = arguments.at(0);
             if (direction == "reset")
             {
                 getSettings()->uiScale.setValue(1);
                 return "";
             }

             if (direction == "in")
             {
                 change = 0.1f;
             }
             else if (direction == "out")
             {
                 change = -0.1f;
             }
             else
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid zoom direction, use \"in\", \"out\", or "
                        "\"reset\"";
                 return "Invalid zoom direction, use \"in\", \"out\", or "
                        "\"reset\"";
             }
             getSettings()->setClampedUiScale(
                 getSettings()->getClampedUiScale() + change);
             return "";
         }},
        {"newTab",
         [this](std::vector<QString>) -> QString {
             this->notebook_->addPage(true);
             return "";
         }},
        {"removeTab",
         [this](std::vector<QString>) -> QString {
             this->notebook_->removeCurrentPage();
             return "";
         }},
        {"reopenSplit",
         [this](std::vector<QString>) -> QString {
             if (ClosedSplits::empty())
             {
                 return "";
             }
             ClosedSplits::SplitInfo si = ClosedSplits::pop();
             SplitContainer *splitContainer{nullptr};
             if (si.tab)
             {
                 splitContainer = dynamic_cast<SplitContainer *>(si.tab->page);
             }
             if (!splitContainer)
             {
                 splitContainer = this->notebook_->getOrAddSelectedPage();
             }
             Split *split = new Split(splitContainer);
             split->setChannel(
                 getApp()->twitch->getOrAddChannel(si.channelName));
             split->setFilters(si.filters);
             splitContainer->insertSplit(split);
             splitContainer->setSelected(split);
             this->notebook_->select(splitContainer);
             return "";
         }},
        {"toggleLocalR9K",
         [](std::vector<QString>) -> QString {
             getSettings()->hideSimilar.setValue(!getSettings()->hideSimilar);
             getApp()->windows->forceLayoutChannelViews();
             return "";
         }},
        {"openQuickSwitcher",
         [this](std::vector<QString>) -> QString {
             auto *quickSwitcher = new QuickSwitcherPopup(this);
             quickSwitcher->show();
             return "";
         }},
        {"quit",
         [](std::vector<QString>) -> QString {
             QApplication::exit();
             return "";
         }},
        {"moveTab",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "moveTab shortcut called without arguments. "
                        "Takes only one argument: new index (number, "
                        "\"next\" "
                        "or \"previous\")";
                 return "moveTab shortcut called without arguments. "
                        "Takes only one argument: new index (number, "
                        "\"next\" "
                        "or \"previous\")";
             }
             int newIndex = -1;
             bool indexIsGenerated =
                 false;  // indicates if `newIndex` was generated using target="next" or target="previous"

             auto target = arguments.at(0);
             qCDebug(chatterinoHotkeys) << target;
             if (target == "next")
             {
                 newIndex = this->notebook_->getSelectedIndex() + 1;
                 indexIsGenerated = true;
             }
             else if (target == "previous")
             {
                 newIndex = this->notebook_->getSelectedIndex() - 1;
                 indexIsGenerated = true;
             }
             else
             {
                 bool ok;
                 int result = target.toInt(&ok);
                 if (!ok)
                 {
                     qCWarning(chatterinoHotkeys)
                         << "Invalid argument for moveTab shortcut";
                     return QString("Invalid argument for moveTab shortcut: "
                                    "%1. Use \"next\" or \"previous\" or an "
                                    "integer.")
                         .arg(target);
                 }
                 newIndex = result;
             }
             if (newIndex >= this->notebook_->getPageCount() || 0 > newIndex)
             {
                 if (indexIsGenerated)
                 {
                     return "";  // don't error out on generated indexes, ie move tab right
                 }
                 qCWarning(chatterinoHotkeys)
                     << "Invalid index for moveTab shortcut:" << newIndex;
                 return QString("Invalid index for moveTab shortcut: %1.")
                     .arg(newIndex);
             }
             this->notebook_->rearrangePage(this->notebook_->getSelectedPage(),
                                            newIndex);
             return "";
         }},
        {"setStreamerMode",
         [](std::vector<QString> arguments) -> QString {
             auto mode = 2;
             if (arguments.size() != 0)
             {
                 auto arg = arguments.at(0);
                 if (arg == "off")
                 {
                     mode = 0;
                 }
                 else if (arg == "on")
                 {
                     mode = 1;
                 }
                 else if (arg == "toggle")
                 {
                     mode = 2;
                 }
                 else if (arg == "auto")
                 {
                     mode = 3;
                 }
                 else
                 {
                     qCWarning(chatterinoHotkeys)
                         << "Invalid argument for setStreamerMode hotkey: "
                         << arg;
                     return QString("Invalid argument for setStreamerMode "
                                    "hotkey: %1. Use \"on\", \"off\", "
                                    "\"toggle\" or \"auto\".")
                         .arg(arg);
                 }
             }

             if (mode == 0)
             {
                 getSettings()->enableStreamerMode.setValue(
                     StreamerModeSetting::Disabled);
             }
             else if (mode == 1)
             {
                 getSettings()->enableStreamerMode.setValue(
                     StreamerModeSetting::Enabled);
             }
             else if (mode == 2)
             {
                 if (isInStreamerMode())
                 {
                     getSettings()->enableStreamerMode.setValue(
                         StreamerModeSetting::Disabled);
                 }
                 else
                 {
                     getSettings()->enableStreamerMode.setValue(
                         StreamerModeSetting::Enabled);
                 }
             }
             else if (mode == 3)
             {
                 getSettings()->enableStreamerMode.setValue(
                     StreamerModeSetting::DetectStreamingSoftware);
             }
             return "";
         }},
        {"setTabVisibility",
         [this](std::vector<QString> arguments) -> QString {
             QString arg = arguments.empty() ? "toggle" : arguments.front();

             if (arg == "off")
             {
                 this->notebook_->setShowTabs(false);
                 getSettings()->tabVisibility.setValue(
                     NotebookTabVisibility::AllTabs);
             }
             else if (arg == "on")
             {
                 this->notebook_->setShowTabs(true);
                 getSettings()->tabVisibility.setValue(
                     NotebookTabVisibility::AllTabs);
             }
             else if (arg == "toggle")
             {
                 this->notebook_->setShowTabs(!this->notebook_->getShowTabs());
                 getSettings()->tabVisibility.setValue(
                     NotebookTabVisibility::AllTabs);
             }
             else if (arg == "liveOnly")
             {
                 this->notebook_->setShowTabs(true);
                 getSettings()->tabVisibility.setValue(
                     NotebookTabVisibility::LiveOnly);
             }
             else if (arg == "toggleLiveOnly")
             {
                 if (!this->notebook_->getShowTabs())
                 {
                     // Tabs are currently hidden, so the intention is to show
                     // tabs again before enabling the live only setting
                     this->notebook_->setShowTabs(true);
                     getSettings()->tabVisibility.setValue(
                         NotebookTabVisibility::LiveOnly);
                 }
                 else
                 {
                     getSettings()->tabVisibility.setValue(
                         getSettings()->tabVisibility.getEnum() ==
                                 NotebookTabVisibility::LiveOnly
                             ? NotebookTabVisibility::AllTabs
                             : NotebookTabVisibility::LiveOnly);
                 }
             }
             else
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid argument for setTabVisibility hotkey: " << arg;
                 return QString("Invalid argument for setTabVisibility hotkey: "
                                "%1. Use \"on\", \"off\", \"toggle\", "
                                "\"liveOnly\", or \"toggleLiveOnly\".")
                     .arg(arg);
             }

             return "";
         }},
    };

    this->addDebugStuff(actions);

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
        HotkeyCategory::Window, actions, this);
}

void Window::addMenuBar()
{
    QMenuBar *mainMenu = new QMenuBar();
    mainMenu->setNativeMenuBar(true);

    // First menu.
    QMenu *menu = mainMenu->addMenu(QString());
    QAction *prefs = menu->addAction(QString());
    prefs->setMenuRole(QAction::PreferencesRole);
    connect(prefs, &QAction::triggered, this, [this] {
        SettingsDialog::showDialog(this);
    });

    // Window menu.
    QMenu *windowMenu = mainMenu->addMenu(QString("Window"));

    QAction *nextTab = windowMenu->addAction(QString("Select next tab"));
    nextTab->setShortcuts({QKeySequence("Meta+Tab")});
    connect(nextTab, &QAction::triggered, this, [this] {
        this->notebook_->selectNextTab();
    });

    QAction *prevTab = windowMenu->addAction(QString("Select previous tab"));
    prevTab->setShortcuts({QKeySequence("Meta+Shift+Tab")});
    connect(prevTab, &QAction::triggered, this, [this] {
        this->notebook_->selectPreviousTab();
    });
}

void Window::onAccountSelected()
{
    auto user = getApp()->accounts->twitch.getCurrent();

    // update title (also append username on Linux and MacOS)
    QString windowTitle = Version::instance().fullVersion();

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    if (user->isAnon())
    {
        windowTitle += " - not logged in";
    }
    else
    {
        windowTitle += " - " + user->getUserName();
    }
#endif

    this->setWindowTitle(windowTitle);

    // update user
    if (this->userLabel_)
    {
        if (user->isAnon())
        {
            this->userLabel_->getLabel().setText("anonymous");
        }
        else
        {
            this->userLabel_->getLabel().setText(user->getUserName());
        }
    }
}

}  // namespace chatterino
