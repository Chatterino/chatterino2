#include "widgets/Window.hpp"

#include "Application.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Updates.hpp"
#include "singletons/WindowManager.hpp"
#include "util/InitUpdateButton.hpp"
#include "util/Shortcut.hpp"
#include "widgets/AccountSwitchPopupWidget.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/dialogs/UpdateDialog.hpp"
#include "widgets/dialogs/WelcomeDialog.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/TitlebarButton.hpp"
#include "widgets/splits/ClosedSplits.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QHeaderView>
#include <QPalette>
#include <QShortcut>
#include <QVBoxLayout>

#include <QStandardItemModel>

namespace chatterino {

Window::Window(WindowType type)
    : BaseWindow(nullptr, BaseWindow::EnableCustomFrame)
    , type_(type)
    , notebook_(new SplitNotebook(this))
{
    this->addCustomTitlebarButtons();
    this->addDebugStuff();
    this->addShortcuts();
    this->addLayout();

    this->signalHolder_.managedConnect(
        getApp()->accounts->twitch.currentUserChanged,
        [this] { this->onAccountSelected(); });
    this->onAccountSelected();

    if (type == WindowType::Main)
    {
        this->resize(int(600 * this->scale()), int(500 * this->scale()));
    }
    else
    {
        this->resize(int(300 * this->scale()), int(500 * this->scale()));
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
        case QEvent::WindowActivate:
            break;

        case QEvent::WindowDeactivate:
        {
            auto page = this->notebook_->getOrAddSelectedPage();

            if (page != nullptr)
            {
                std::vector<Split *> splits = page->getSplits();

                for (Split *split : splits)
                {
                    split->updateLastReadMessage();
                }
            }

            if (SplitContainer *container =
                    dynamic_cast<SplitContainer *>(page))
            {
                container->hideResizeHandles();
            }
        }
        break;

        default:;
    };

    return BaseWindow::event(event);
}

void Window::showEvent(QShowEvent *event)
{
    // Startup notification
    if (getSettings()->startUpNotification.getValue() < 1)
    {
        getSettings()->startUpNotification = 1;

        auto box = new QMessageBox(
            QMessageBox::Information, "Chatterino 2 Beta",
            "Please note that this software is not stable yet. Things are "
            "rough "
            "around the edges and everything is subject to change.");
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();
    }

    // Show changelog
    if (getSettings()->currentVersion.getValue() != "" &&
        getSettings()->currentVersion.getValue() != CHATTERINO_VERSION)
    {
        auto box = new QMessageBox(QMessageBox::Information,
                                   "Chatterino 2 Beta", "Show changelog?",
                                   QMessageBox::Yes | QMessageBox::No);
        box->setAttribute(Qt::WA_DeleteOnClose);
        if (box->exec() == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(
                QUrl("https://fourtf.com/chatterino-changelog/"));
        }
    }

    getSettings()->currentVersion.setValue(CHATTERINO_VERSION);

    // --
    BaseWindow::showEvent(event);
}

void Window::closeEvent(QCloseEvent *)
{
    if (this->type_ == WindowType::Main)
    {
        auto app = getApp();
        app->windows->save();
        app->windows->closeAll();
    }

    this->closed.invoke();

    if (this->type_ == WindowType::Main)
    {
        QApplication::exit();
    }
}

void Window::addLayout()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(this->notebook_);
    this->getLayoutContainer()->setLayout(layout);

    // set margin
    layout->setMargin(0);

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
    this->addTitleBarButton(TitleBarButtonStyle::Settings,
                            [] { getApp()->windows->showSettingsDialog(); });

    // updates
    auto update = this->addTitleBarButton(TitleBarButtonStyle::None, [] {});

    initUpdateButton(*update, this->signalHolder_);

    // account
    this->userLabel_ = this->addTitleBarLabel([this] {
        getApp()->windows->showAccountSelectPopup(this->userLabel_->mapToGlobal(
            this->userLabel_->rect().bottomLeft()));
    });
    this->userLabel_->setMinimumWidth(20 * scale());
}

void Window::addDebugStuff()
{
#ifdef QT_DEBUG
    std::vector<QString> cheerMessages, subMessages, miscMessages;
    // clang-format off
    cheerMessages.emplace_back(R"(@badges=subscriber/12,premium/1;bits=2000;color=#B22222;display-name=arzenhuz;emotes=185989:33-37;id=1ae336ac-8e1a-4d6b-8b00-9fcee26e8337;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515783470139;turbo=0;user-id=111553331;user-type= :arzenhuz!arzenhuz@arzenhuz.tmi.twitch.tv PRIVMSG #pajlada :pajacheer2000 Buy pizza for both pajaH)");
    cheerMessages.emplace_back(R"(@badges=subscriber/12,premium/1;bits=37;color=#3FBF72;display-name=VADIKUS007;emotes=;id=eedd95fd-2a17-4da1-879c-a1e76ffce582;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515783184352;turbo=0;user-id=72256775;user-type= :vadikus007!vadikus007@vadikus007.tmi.twitch.tv PRIVMSG #pajlada :cheer37)");
    cheerMessages.emplace_back(R"(@badges=moderator/1,subscriber/24,bits/100;bits=1;color=#DCD3E6;display-name=swiftapples;emotes=80803:7-13;id=1c4647f6-f1a8-4acc-a9b2-b5d23d91258d;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1515538318854;turbo=0;user-id=80526177;user-type=mod :swiftapples!swiftapples@swiftapples.tmi.twitch.tv PRIVMSG #pajlada :cheer1 pajaHey)");
    cheerMessages.emplace_back(R"(@badges=subscriber/12,turbo/1;bits=1;color=#0A2927;display-name=Binkelderk;emotes=;id=a1d9bdc6-6f6a-4c03-8554-d5b34721a878;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515538899479;turbo=1;user-id=89081828;user-type= :binkelderk!binkelderk@binkelderk.tmi.twitch.tv PRIVMSG #pajlada :pajacheer1)");
    cheerMessages.emplace_back(R"(@badges=moderator/1,subscriber/24,bits/100;bits=1;color=#DCD3E6;display-name=swiftapples;emotes=80803:6-12;id=e9e21793-0b58-4ac6-8a1e-c19e165dbc9f;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1515539073209;turbo=0;user-id=80526177;user-type=mod :swiftapples!swiftapples@swiftapples.tmi.twitch.tv PRIVMSG #pajlada :bday1 pajaHey)");
    cheerMessages.emplace_back(R"(@badges=partner/1;bits=1;color=#CC44FF;display-name=pajlada;emotes=;id=ca89214e-4fb5-48ec-853e-d2e6b41355ea;mod=0;room-id=39705480;subscriber=0;tmi-sent-ts=1515546977622;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #leesherwhy :test123 owocheer1 456test)");
    cheerMessages.emplace_back(R"(@badges=subscriber/12,premium/1;bits=1;color=#3FBF72;display-name=VADIKUS007;emotes=;id=c4c5061b-f5c6-464b-8bff-7f1ac816caa7;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515782817171;turbo=0;user-id=72256775;user-type= :vadikus007!vadikus007@vadikus007.tmi.twitch.tv PRIVMSG #pajlada :trihard1)");
    cheerMessages.emplace_back(R"(@badges=;bits=1;color=#FF0000;display-name=?????;emotes=;id=979b6b4f-be9a-42fb-a54c-88fcb0aca18d;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1515782819084;turbo=0;user-id=70656218;user-type= :stels_tv!stels_tv@stels_tv.tmi.twitch.tv PRIVMSG #pajlada :trihard1)");
    cheerMessages.emplace_back(R"(@badges=subscriber/3,premium/1;bits=1;color=#FF0000;display-name=kalvarenga;emotes=;id=4744d6f0-de1d-475d-a3ff-38647113265a;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515782860740;turbo=0;user-id=108393131;user-type= :kalvarenga!kalvarenga@kalvarenga.tmi.twitch.tv PRIVMSG #pajlada :trihard1)");

    subMessages.emplace_back(R"(@badges=staff/1,broadcaster/1,turbo/1;color=#008000;display-name=ronni;emotes=;id=db25007f-7a18-43eb-9379-80131e44d633;login=ronni;mod=0;msg-id=resub;msg-param-months=6;msg-param-sub-plan=Prime;msg-param-sub-plan-name=Prime;room-id=1337;subscriber=1;system-msg=ronni\shas\ssubscribed\sfor\s6\smonths!;tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=staff :tmi.twitch.tv USERNOTICE #pajlada :Great stream -- keep it up!)");
    subMessages.emplace_back(R"(@badges=staff/1,premium/1;color=#0000FF;display-name=TWW2;emotes=;id=e9176cd8-5e22-4684-ad40-ce53c2561c5e;login=tww2;mod=0;msg-id=subgift;msg-param-months=1;msg-param-recipient-display-name=Mr_Woodchuck;msg-param-recipient-id=89614178;msg-param-recipient-name=mr_woodchuck;msg-param-sub-plan-name=House\sof\sNyoro~n;msg-param-sub-plan=1000;room-id=19571752;subscriber=0;system-msg=TWW2\sgifted\sa\sTier\s1\ssub\sto\sMr_Woodchuck!;tmi-sent-ts=1521159445153;turbo=0;user-id=13405587;user-type=staff :tmi.twitch.tv USERNOTICE #pajlada)");

    // hyperbolicxd gifted a sub to quote_if_nam
    subMessages.emplace_back(R"(@badges=subscriber/0,premium/1;color=#00FF7F;display-name=hyperbolicxd;emotes=;id=b20ef4fe-cba8-41d0-a371-6327651dc9cc;login=hyperbolicxd;mod=0;msg-id=subgift;msg-param-months=1;msg-param-recipient-display-name=quote_if_nam;msg-param-recipient-id=217259245;msg-param-recipient-user-name=quote_if_nam;msg-param-sender-count=1;msg-param-sub-plan-name=Channel\sSubscription\s(nymn_hs);msg-param-sub-plan=1000;room-id=62300805;subscriber=1;system-msg=hyperbolicxd\sgifted\sa\sTier\s1\ssub\sto\squote_if_nam!\sThis\sis\stheir\sfirst\sGift\sSub\sin\sthe\schannel!;tmi-sent-ts=1528190938558;turbo=0;user-id=111534250;user-type= :tmi.twitch.tv USERNOTICE #pajlada)");

    // first time sub
    subMessages.emplace_back(R"(@badges=subscriber/0,premium/1;color=#0000FF;display-name=byebyeheart;emotes=;id=fe390424-ab89-4c33-bb5a-53c6e5214b9f;login=byebyeheart;mod=0;msg-id=sub;msg-param-months=0;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=Prime;room-id=39298218;subscriber=0;system-msg=byebyeheart\sjust\ssubscribed\swith\sTwitch\sPrime!;tmi-sent-ts=1528190963670;turbo=0;user-id=131956000;user-type= :tmi.twitch.tv USERNOTICE #pajlada)");

    // first time sub
    subMessages.emplace_back(R"(@badges=subscriber/0,premium/1;color=;display-name=vJoeyzz;emotes=;id=b2476df5-fffe-4338-837b-380c5dd90051;login=vjoeyzz;mod=0;msg-id=sub;msg-param-months=0;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=Prime;room-id=39298218;subscriber=0;system-msg=vJoeyzz\sjust\ssubscribed\swith\sTwitch\sPrime!;tmi-sent-ts=1528190995089;turbo=0;user-id=78945903;user-type= :tmi.twitch.tv USERNOTICE #pajlada)");

    // first time sub
    subMessages.emplace_back(R"(@badges=subscriber/0,premium/1;color=;display-name=Lennydog3;emotes=;id=44feb1eb-df60-45f6-904b-7bf0d5375a41;login=lennydog3;mod=0;msg-id=sub;msg-param-months=0;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=Prime;room-id=39298218;subscriber=0;system-msg=Lennydog3\sjust\ssubscribed\swith\sTwitch\sPrime!;tmi-sent-ts=1528191098733;turbo=0;user-id=175759335;user-type= :tmi.twitch.tv USERNOTICE #pajlada)");

    // resub with message
    subMessages.emplace_back(R"(@badges=subscriber/0,premium/1;color=#1E90FF;display-name=OscarLord;emotes=;id=376529fd-31a8-4da9-9c0d-92a9470da2cd;login=oscarlord;mod=0;msg-id=resub;msg-param-months=2;msg-param-sub-plan-name=Dakotaz;msg-param-sub-plan=1000;room-id=39298218;subscriber=1;system-msg=OscarLord\sjust\ssubscribed\swith\sa\sTier\s1\ssub.\sOscarLord\ssubscribed\sfor\s2\smonths\sin\sa\srow!;tmi-sent-ts=1528191154801;turbo=0;user-id=162607810;user-type= :tmi.twitch.tv USERNOTICE #pajlada :Hey dk love to watch your streams keep up the good work)");

    // resub with message
    subMessages.emplace_back(R"(@badges=subscriber/0,premium/1;color=;display-name=samewl;emotes=9:22-23;id=599fda87-ca1e-41f2-9af7-6a28208daf1c;login=samewl;mod=0;msg-id=resub;msg-param-months=5;msg-param-sub-plan-name=Channel\sSubscription\s(forsenlol);msg-param-sub-plan=Prime;room-id=22484632;subscriber=1;system-msg=samewl\sjust\ssubscribed\swith\sTwitch\sPrime.\ssamewl\ssubscribed\sfor\s5\smonths\sin\sa\srow!;tmi-sent-ts=1528191317948;turbo=0;user-id=70273207;user-type= :tmi.twitch.tv USERNOTICE #pajlada :lot of love sebastian <3)");

    // resub without message
    subMessages.emplace_back(R"(@badges=subscriber/12;color=#CC00C2;display-name=cspice;emotes=;id=6fc4c3e0-ca61-454a-84b8-5669dee69fc9;login=cspice;mod=0;msg-id=resub;msg-param-months=12;msg-param-sub-plan-name=Channel\sSubscription\s(forsenlol):\s$9.99\sSub;msg-param-sub-plan=2000;room-id=22484632;subscriber=1;system-msg=cspice\sjust\ssubscribed\swith\sa\sTier\s2\ssub.\scspice\ssubscribed\sfor\s12\smonths\sin\sa\srow!;tmi-sent-ts=1528192510808;turbo=0;user-id=47894662;user-type= :tmi.twitch.tv USERNOTICE #pajlada)");

    // display name renders strangely
    miscMessages.emplace_back(R"(@badges=;color=#00AD2B;display-name=Iamme420\s;emotes=;id=d47a1e4b-a3c6-4b9e-9bf1-51b8f3dbc76e;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1529670347537;turbo=0;user-id=56422869;user-type= :iamme420!iamme420@iamme420.tmi.twitch.tv PRIVMSG #pajlada :offline chat gachiBASS)");
    // clang-format on

    createWindowShortcut(this, "F6", [=] {
        const auto &messages = miscMessages;
        static int index = 0;
        auto app = getApp();
        const auto &msg = messages[index++ % messages.size()];
        app->twitch.server->addFakeMessage(msg);
    });

    createWindowShortcut(this, "F9", [=] {
        auto *dialog = new WelcomeDialog();
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
#endif
}

void Window::addShortcuts()
{
    /// Initialize program-wide hotkeys
    // Open settings
    createWindowShortcut(this, "CTRL+P", [] { SettingsDialog::showDialog(); });

    // Switch tab
    createWindowShortcut(this, "CTRL+T", [this] {
        this->notebook_->getOrAddSelectedPage()->appendNewSplit(true);
    });

    createWindowShortcut(this, "CTRL+1",
                         [this] { this->notebook_->selectIndex(0); });
    createWindowShortcut(this, "CTRL+2",
                         [this] { this->notebook_->selectIndex(1); });
    createWindowShortcut(this, "CTRL+3",
                         [this] { this->notebook_->selectIndex(2); });
    createWindowShortcut(this, "CTRL+4",
                         [this] { this->notebook_->selectIndex(3); });
    createWindowShortcut(this, "CTRL+5",
                         [this] { this->notebook_->selectIndex(4); });
    createWindowShortcut(this, "CTRL+6",
                         [this] { this->notebook_->selectIndex(5); });
    createWindowShortcut(this, "CTRL+7",
                         [this] { this->notebook_->selectIndex(6); });
    createWindowShortcut(this, "CTRL+8",
                         [this] { this->notebook_->selectIndex(7); });
    createWindowShortcut(this, "CTRL+9",
                         [this] { this->notebook_->selectIndex(8); });

    // Zoom in
    {
        auto s = new QShortcut(QKeySequence::ZoomIn, this);
        s->setContext(Qt::WindowShortcut);
        QObject::connect(s, &QShortcut::activated, this, [] {
            getSettings()->setClampedUiScale(
                getSettings()->getClampedUiScale() + 0.1f);
        });
    }

    // Zoom out
    {
        auto s = new QShortcut(QKeySequence::ZoomOut, this);
        s->setContext(Qt::WindowShortcut);
        QObject::connect(s, &QShortcut::activated, this, [] {
            getSettings()->setClampedUiScale(
                getSettings()->getClampedUiScale() - 0.1f);
        });
    }

    // New tab
    createWindowShortcut(this, "CTRL+SHIFT+T",
                         [this] { this->notebook_->addPage(true); });

    // Close tab
    createWindowShortcut(this, "CTRL+SHIFT+W",
                         [this] { this->notebook_->removeCurrentPage(); });

    // Reopen last closed split
    createWindowShortcut(this, "CTRL+G", [this] {
        if (ClosedSplits::empty())
        {
            return;
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
        this->notebook_->select(splitContainer);
        Split *split = new Split(splitContainer);
        split->setChannel(
            getApp()->twitch.server->getOrAddChannel(si.channelName));
        splitContainer->appendSplit(split);
    });
}

#define UGLYMACROHACK1(s) #s
#define UGLYMACROHACK(s) UGLYMACROHACK1(s)

void Window::onAccountSelected()
{
    auto user = getApp()->accounts->twitch.getCurrent();

#ifdef CHATTERINO_NIGHTLY_VERSION_STRING
    auto windowTitleEnd =
        QString("Chatterino Nightly " CHATTERINO_VERSION
                " (" UGLYMACROHACK(CHATTERINO_NIGHTLY_VERSION_STRING) ")");
#else
    auto windowTitleEnd = QString("Chatterino Beta " CHATTERINO_VERSION);
#endif

    this->setWindowTitle(windowTitleEnd);

    if (user->isAnon())
    {
        if (this->userLabel_)
        {
            this->userLabel_->getLabel().setText("anonymous");
        }
    }
    else
    {
        if (this->userLabel_)
        {
            this->userLabel_->getLabel().setText(user->getUserName());
        }
    }
}

}  // namespace chatterino
