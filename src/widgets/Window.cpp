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

#include <QMenuBar>
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

#ifdef Q_OS_MACOS
    this->addMenuBar();
#endif

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

        // auto box = new QMessageBox(
        //     QMessageBox::Information, "Chatterino 2 Beta",
        //     "Please note that this software is not stable yet. Things are "
        //     "rough "
        //     "around the edges and everything is subject to change.");
        // box->setAttribute(Qt::WA_DeleteOnClose);
        // box->show();
    }

    // Show changelog
    if (getSettings()->currentVersion.getValue() != "" &&
        getSettings()->currentVersion.getValue() != CHATTERINO_VERSION)
    {
        auto box = new QMessageBox(QMessageBox::Information, "Chatterino 2",
                                   "Show changelog?",
                                   QMessageBox::Yes | QMessageBox::No);
        box->setAttribute(Qt::WA_DeleteOnClose);
        if (box->exec() == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(
                QUrl("https://www.chatterino.com/changelog"));
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
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#008000;display-name=dGAussie;emotes=;flags=;id=5fb5ed1c-4e18-4a03-9cc7-4a69b7fedff7;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1566783370478;turbo=0;user-id=37141440;user-type= :dgaussie!dgaussie@dgaussie.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 W00t podcast)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=1;color=#FF0000;display-name=FlameGodFlann;emotes=;flags=;id=93a63505-82f1-4a41-b573-2c63d6224db4;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1567283183354;turbo=0;user-id=56442185;user-type= :flamegodflann!flamegodflann@flamegodflann.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 The Stella's gone back to the fridge, till nextime James!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=1;color=#FF0000;display-name=69_faith_420;emotes=300443490:120-134;flags=;id=965515ff-c387-4779-9e85-cfb8c2e12423;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1567283108855;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer1 Lovely stream today! thanks for putting up with me, sorry if I talked too much. Take care tonight, stay hydrated comple42Hydrate)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=moderator/1,subscriber/3,premium/1;bits=10;color=;display-name=Shedjunk;emotes=;flags=;id=af2a8991-9c85-456d-a6e7-cf7d909fa028;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1567283104905;turbo=0;user-id=52825550;user-type=mod :shedjunk!shedjunk@shedjunk.tmi.twitch.tv PRIVMSG #pajlada :@completeditmate Cheer10 Get your gums round these you bloody love albatross)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=1;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=d525a080-456c-419a-80e6-80ee9788f826;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1567282409412;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer1 By the way I've eaten about two slices of my pizza so far... ¯\_(ツ)_/¯)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=2;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=c5fd49c7-ecbc-46dd-a790-c9f10fdaaa67;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1567282184553;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer2 Stop what? I'm not doing anything.)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=2;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=397f4d2e-cac8-4689-922a-32709b9e8b4f;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1567282159076;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer2 Who keeps getting their bits out now?)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=2;color=#FF0000;display-name=FlameGodFlann;emotes=;flags=;id=664ddc92-649d-4889-9641-208a6e62ef1e;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1567282066199;turbo=0;user-id=56442185;user-type= :flamegodflann!flamegodflann@flamegodflann.tmi.twitch.tv PRIVMSG #pajlada :Cheer2 I'm saving my only can of Stella for your upcoming win, lets go!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/3,bits/100;bits=10;color=#008000;display-name=k4izn;emotes=;flags=;id=3919af0b-93e0-412c-b238-d152f92ffea7;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1567811485257;turbo=0;user-id=207114672;user-type=mod :k4izn!k4izn@k4izn.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Kleiner Cheer(s) !)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/12;badges=subscriber/12,bits/1000;bits=20;color=#00CCFF;display-name=YaBoiBurnsy;emotes=;flags=;id=5b53975d-b339-484f-a2a0-3ffbedde0df2;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1567529634584;turbo=0;user-id=45258137;user-type= :yaboiburnsy!yaboiburnsy@yaboiburnsy.tmi.twitch.tv PRIVMSG #pajlada :ShowLove20)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/10000;bits=10000;color=;display-name=mathmaru;emotes=;flags=;id=00318434-150c-4a3b-8310-baa82942e7e9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567273637572;turbo=0;user-id=151009486;user-type= :mathmaru!mathmaru@mathmaru.tmi.twitch.tv PRIVMSG #pajlada :cheer10000)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#008000;display-name=dGAussie;emotes=;flags=;id=5fb5ed1c-4e18-4a03-9cc7-4a69b7fedff7;mod=0;room-id=111449917;subscriber=0;tmi-sent-ts=1566783370478;turbo=0;user-id=37141440;user-type= :dgaussie!dgaussie@dgaussie.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 W00t podcast)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=1;color=#FF0000;display-name=FlameGodFlann;emotes=;flags=;id=93a63505-82f1-4a41-b573-2c63d6224db4;mod=0;room-id=111449917;subscriber=1;tmi-sent-ts=1567283183354;turbo=0;user-id=56442185;user-type= :flamegodflann!flamegodflann@flamegodflann.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 The Stella's gone back to the fridge, till nextime James!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=1;color=#FF0000;display-name=69_faith_420;emotes=300443490:120-134;flags=;id=965515ff-c387-4779-9e85-cfb8c2e12423;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567283108855;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer1 Lovely stream today! thanks for putting up with me, sorry if I talked too much. Take care tonight, stay hydrated comple42Hydrate)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=moderator/1,subscriber/3,premium/1;bits=10;color=;display-name=Shedjunk;emotes=;flags=;id=af2a8991-9c85-456d-a6e7-cf7d909fa028;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567283104905;turbo=0;user-id=52825550;user-type=mod :shedjunk!shedjunk@shedjunk.tmi.twitch.tv PRIVMSG #pajlada :@completeditmate Cheer10 Get your gums round these you bloody love albatross)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=1;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=d525a080-456c-419a-80e6-80ee9788f826;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567282409412;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer1 By the way I've eaten about two slices of my pizza so far... ¯\_(ツ)_/¯)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=2;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=c5fd49c7-ecbc-46dd-a790-c9f10fdaaa67;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567282184553;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer2 Stop what? I'm not doing anything.)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=moderator/1,subscriber/3,sub-gifter/5;bits=2;color=#FF0000;display-name=69_faith_420;emotes=;flags=;id=397f4d2e-cac8-4689-922a-32709b9e8b4f;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567282159076;turbo=0;user-id=125608098;user-type=mod :69_faith_420!69_faith_420@69_faith_420.tmi.twitch.tv PRIVMSG #pajlada :cheer2 Who keeps getting their bits out now?)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=2;color=#FF0000;display-name=FlameGodFlann;emotes=;flags=;id=664ddc92-649d-4889-9641-208a6e62ef1e;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567282066199;turbo=0;user-id=56442185;user-type= :flamegodflann!flamegodflann@flamegodflann.tmi.twitch.tv PRIVMSG #pajlada :Cheer2 I'm saving my only can of Stella for your upcoming win, lets go!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/3,bits/100;bits=10;color=#008000;display-name=k4izn;emotes=;flags=;id=3919af0b-93e0-412c-b238-d152f92ffea7;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567811485257;turbo=0;user-id=207114672;user-type=mod :k4izn!k4izn@k4izn.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Kleiner Cheer(s) !)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/12;badges=subscriber/12,bits/1000;bits=20;color=#00CCFF;display-name=YaBoiBurnsy;emotes=;flags=;id=5b53975d-b339-484f-a2a0-3ffbedde0df2;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567529634584;turbo=0;user-id=45258137;user-type= :yaboiburnsy!yaboiburnsy@yaboiburnsy.tmi.twitch.tv PRIVMSG #pajlada :ShowLove20)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/2;badges=subscriber/0,bits/100;bits=30;color=#1E90FF;display-name=ponta84;emotes=;flags=;id=5ee1dce7-05b4-42e6-bd5a-138220985f2e;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567877788059;turbo=0;user-id=142902675;user-type= :ponta84!ponta84@ponta84.tmi.twitch.tv PRIVMSG #pajlada :cheer30 im nr one)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=moderator/1,subscriber/0,sub-gifter/10;bits=500;color=#FF0000;display-name=iAtlasOG;emotes=;flags=;id=2360883d-39c6-4d26-ac83-9404803b49f7;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567743906079;turbo=0;user-id=183770718;user-type=mod :iatlasog!iatlasog@iatlasog.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 Subway100 Subway100 Subway100 bonus50)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=moderator/1,subscriber/0,bits-leader/2;bits=1;color=;display-name=jdfellie;emotes=;flags=38-42:A.3/P.5;id=4ebbc148-04ef-4873-ab53-5ebe7ceb4c8e;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567743821413;turbo=0;user-id=137619637;user-type=mod :jdfellie!jdfellie@jdfellie.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 not on a hunter...here's a bit bitch)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=moderator/1,subscriber/0,bits-leader/2;bits=1;color=;display-name=jdfellie;emotes=;flags=18-22:A.3/P.5;id=28c8f4b7-b1e3-4404-b0f8-5cfe46411ef9;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567668177856;turbo=0;user-id=137619637;user-type=mod :jdfellie!jdfellie@jdfellie.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 take a bit bitch)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=30;color=#EC3B83;display-name=Sammay;emotes=;flags=;id=ccf058a6-c1f1-45de-a764-fc8f96f21449;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566719874294;turbo=0;user-id=58283830;user-type= :sammay!sammay@sammay.tmi.twitch.tv PRIVMSG #pajlada :ShowLove30 @Emperor_Zhang)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=6;color=#97E7FF;display-name=Emperor_Zhang;emotes=;flags=;id=53bab01b-9f6c-4123-a852-9916ab371cf9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566719803345;turbo=0;user-id=105292882;user-type= :emperor_zhang!emperor_zhang@emperor_zhang.tmi.twitch.tv PRIVMSG #pajlada :uni6)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#EC3B83;display-name=Sammay;emotes=;flags=;id=defda764-0ee0-48fd-954a-522f8259f993;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566719772799;turbo=0;user-id=58283830;user-type= :sammay!sammay@sammay.tmi.twitch.tv PRIVMSG #pajlada :Scoops10)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=5;color=#97E7FF;display-name=Emperor_Zhang;emotes=;flags=;id=545caec6-8b5f-460a-8b4b-3e407e179689;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566704926380;turbo=0;user-id=105292882;user-type= :emperor_zhang!emperor_zhang@emperor_zhang.tmi.twitch.tv PRIVMSG #pajlada :VoHiYo5)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=50;color=;display-name=Schmiddi55;emotes=;flags=;id=777f1018-941d-48aa-bf4e-ed8053d556c8;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567708393343;turbo=0;user-id=101444120;user-type= :schmiddi55!schmiddi55@schmiddi55.tmi.twitch.tv PRIVMSG #pajlada :cheer50 sere ihr radlertrinker)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=subscriber/3,sub-gifter/10;bits=100;color=#0000FF;display-name=MLPTheChad;emotes=;flags=87-91:P.5;id=ed7db31e-884b-4761-9c88-b1676caa8814;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567681752733;turbo=0;user-id=63179867;user-type= :mlpthechad!mlpthechad@mlpthechad.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 Statistically speaking, 10 out of 10 constipated people don't give a shit.)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=subscriber/3,sub-gifter/10;bits=100;color=#0000FF;display-name=MLPTheChad;emotes=;flags=;id=506b482a-515a-4914-a694-2c69d2add23a;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567681618814;turbo=0;user-id=63179867;user-type= :mlpthechad!mlpthechad@mlpthechad.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 That's some SUB par gameplay, Dabier.)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=premium/1;bits=100;color=;display-name=AkiraKurusu__;emotes=;flags=;id=6e343f5d-0e0e-47f7-bf6d-d5d7bf18b95a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567765732657;turbo=0;user-id=151679027;user-type= :akirakurusu__!akirakurusu__@akirakurusu__.tmi.twitch.tv PRIVMSG #pajlada :TriHard100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=premium/1;bits=1;color=;display-name=AkiraKurusu__;emotes=;flags=;id=dfdf6c2f-abee-4a4b-99fe-0d0b221f07de;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567765295301;turbo=0;user-id=151679027;user-type= :akirakurusu__!akirakurusu__@akirakurusu__.tmi.twitch.tv PRIVMSG #pajlada :TriHard1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=500;color=#0000FF;display-name=Stabbr;emotes=;flags=;id=e28b384e-fb6a-4da5-9a36-1b6153c6089d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567648284623;turbo=0;user-id=183081176;user-type= :stabbr!stabbr@stabbr.tmi.twitch.tv PRIVMSG #pajlada :cheer500 Gotta be on top)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=100;color=;display-name=dbf_sub;emotes=;flags=;id=7cf317b8-6e28-4615-a0ba-e0bbaa0d4b29;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567646349560;turbo=0;user-id=450101746;user-type= :dbf_sub!dbf_sub@dbf_sub.tmi.twitch.tv PRIVMSG #pajlada :EleGiggle100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=1;color=;display-name=dbf_sub;emotes=;flags=;id=43b5fc97-e7cc-4ac1-8d7e-7504c435c3f1;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567643510222;turbo=0;user-id=450101746;user-type= :dbf_sub!dbf_sub@dbf_sub.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=100;color=;display-name=RobertsonRobotics;emotes=;flags=;id=598dfa14-23e9-4e45-a2fe-7a0263828817;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567873463820;turbo=0;user-id=117177721;user-type= :robertsonrobotics!robertsonrobotics@robertsonrobotics.tmi.twitch.tv PRIVMSG #pajlada :firstCheer100 This is so cool! Can’t wait for the competition!)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=18;color=#1E90FF;display-name=Vipacman11;emotes=;flags=;id=07f59664-0c75-459e-b137-26c8d03e44be;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567873210379;turbo=0;user-id=89634839;user-type= :vipacman11!vipacman11@vipacman11.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=sub-gifter/5;bits=100;color=#FF7F50;display-name=darkside_sinner;emotes=;flags=;id=090102b3-369d-4ce4-ad1f-283849b10de0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567822075293;turbo=0;user-id=104942909;user-type= :darkside_sinner!darkside_sinner@darkside_sinner.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=sub-gifter/5;bits=200;color=#FF7F50;display-name=darkside_sinner;emotes=;flags=;id=2bdf7846-5ffa-4798-a397-997e7209a6d0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567821695287;turbo=0;user-id=104942909;user-type= :darkside_sinner!darkside_sinner@darkside_sinner.tmi.twitch.tv PRIVMSG #pajlada :Subway200 bonus20)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=50;color=#0000FF;display-name=SincereBC;emotes=;flags=;id=b8c9236b-aeb9-4c72-a191-593e33c6c3f1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567818308913;turbo=0;user-id=146097597;user-type= :sincerebc!sincerebc@sincerebc.tmi.twitch.tv PRIVMSG #pajlada :cheer50)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=#FF0000;display-name=AngryCh33s3puff;emotes=;flags=;id=6ab62185-ac1b-4ee5-bd93-165009917078;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567474810480;turbo=0;user-id=55399500;user-type= :angrych33s3puff!angrych33s3puff@angrych33s3puff.tmi.twitch.tv PRIVMSG #pajlada :cheer1 for the chair!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/17;badges=subscriber/12,bits/1000;bits=100;color=;display-name=Fastkiller1988;emotes=;flags=;id=167474d3-2320-4a53-93b4-2665e531fc64;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567857068187;turbo=0;user-id=178285317;user-type= :fastkiller1988!fastkiller1988@fastkiller1988.tmi.twitch.tv PRIVMSG #pajlada :SwiftRage100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=#B8BAFF;display-name=Wally_KC;emotes=;flags=;id=9bdac196-b4ae-450a-ab7c-1f4da6846139;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567798319597;turbo=0;user-id=78059836;user-type= :wally_kc!wally_kc@wally_kc.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood100 more money for pocket)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=500;color=#DAA520;display-name=SlayersofDiablo;emotes=;flags=;id=eec394aa-8a59-44f8-b5e4-db19caccf9c7;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567860767886;turbo=0;user-id=27649971;user-type= :slayersofdiablo!slayersofdiablo@slayersofdiablo.tmi.twitch.tv PRIVMSG #pajlada :subway500 bonus50 best wishes, friend)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=300;color=#1E90FF;display-name=Persie;emotes=;flags=;id=b3bc797c-2f6c-4187-95d8-0bd337dc6be2;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567860040079;turbo=0;user-id=92880632;user-type= :persie!persie@persie.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Cheer100 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=#9ACD32;display-name=paX_On_Earth;emotes=;flags=;id=998e07c1-e385-4dbd-a21e-80fbf5baace1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567819751966;turbo=0;user-id=37232205;user-type= :pax_on_earth!pax_on_earth@pax_on_earth.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Love ya buddy, take care!!!)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=premium/1;bits=750;color=;display-name=Buggiezor;emotes=;flags=;id=eadaf912-24c3-4cdd-bf82-30b7c5a480d6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567819706179;turbo=0;user-id=191362286;user-type= :buggiezor!buggiezor@buggiezor.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Cheer100 Cheer100 Cheer100 Cheer100 Cheer100 Cheer100 cheer50 take care of yourself and your family!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/36;badges=subscriber/36,bits-leader/2;bits=100;color=#5F9EA0;display-name=CQuO;emotes=;flags=;id=94b22c12-e6b8-423f-9c37-b49e6c224425;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567834264180;turbo=0;user-id=29753653;user-type= :cquo!cquo@cquo.tmi.twitch.tv PRIVMSG #pajlada :cheer100 just fly to wetlands and go on the left boat to theramore)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=6;color=#008000;display-name=mochalinky;emotes=;flags=;id=b6365551-9937-4f4b-ab72-adf43669acfa;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567472540403;turbo=0;user-id=157854241;user-type= :mochalinky!mochalinky@mochalinky.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#2E8B57;display-name=PC12Junkie;emotes=;flags=;id=09cdb5bb-eff7-4e79-998a-456cbf64369d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567199611611;turbo=0;user-id=163405976;user-type= :pc12junkie!pc12junkie@pc12junkie.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 awesome strimmer)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/30;badges=vip/1,subscriber/24,bits/10000;bits=100;color=#00FF7F;display-name=ekothelux;emotes=3:151-152;flags=;id=3615fc4f-1ed7-4b01-b72b-90f67713c8ab;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567140348206;turbo=0;user-id=115184471;user-type= :ekothelux!ekothelux@ekothelux.tmi.twitch.tv PRIVMSG #pajlada :jewelCheer100 Well Megsy! i will see you soon. i'll still be on twitch but will be resting a little. must say i missed you alot! much those biddieeees :D love ya)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/17;badges=subscriber/12;bits=2000;color=#81E97D;display-name=lazykiller23rd;emotes=;flags=;id=7201e5e3-7c19-499f-8814-3e3ee722e3f1;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567134912291;turbo=0;user-id=119407651;user-type= :lazykiller23rd!lazykiller23rd@lazykiller23rd.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1000 ShowLove1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/17;badges=moderator/1,subscriber/12,twitchconNA2019/1;bits=200;color=#EF0DC2;display-name=PottsieTV;emotes=;flags=;id=6f5d945b-40eb-446b-b69d-de62a18ba0e7;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567639215200;turbo=0;user-id=176850556;user-type=mod :pottsietv!pottsietv@pottsietv.tmi.twitch.tv PRIVMSG #pajlada :Kappa200)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=;display-name=chaoticmage7996;emotes=;flags=;id=f594c75a-263a-448e-971f-2726c3158cd4;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566776112551;turbo=0;user-id=225336970;user-type= :chaoticmage7996!chaoticmage7996@chaoticmage7996.tmi.twitch.tv PRIVMSG #pajlada :Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=moderator/1,bits-leader/1;bits=1;color=#0008FF;display-name=joao6067;emotes=;flags=;id=670a8264-8c1b-4208-a61f-0a72caf51952;mod=1;room-id=111448817;subscriber=0;tmi-sent-ts=1566774757129;turbo=0;user-id=127484325;user-type=mod :joao6067!joao6067@joao6067.tmi.twitch.tv PRIVMSG #pajlada :boas cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=premium/1;bits=4;color=;display-name=clink7202088;emotes=;flags=;id=8fdf8fb7-bea6-4909-ad0e-2b2ca70e61c6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567881933827;turbo=0;user-id=451965633;user-type= :clink7202088!clink7202088@clink7202088.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1000;bits=300;color=#B22222;display-name=THEBOSSHOGG;emotes=;flags=;id=7514da79-9334-43b3-91d3-c496ee31b41a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567816918967;turbo=0;user-id=52532773;user-type= :thebosshogg!thebosshogg@thebosshogg.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 Subway100 bonus30 purr for me carrot lol)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=;display-name=bobbrieko;emotes=;flags=;id=cfe75753-e44b-4257-bba4-b0d30cc7dfc3;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567818544141;turbo=0;user-id=161030571;user-type= :bobbrieko!bobbrieko@bobbrieko.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 happy early birthday)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/60;badges=moderator/1,subscriber/48,bits/400000;bits=1500;color=#4ECC00;display-name=NOTanotherTACO;emotes=167597:76-84/106623:86-93/351004:95-105/15976:107-114;flags=;id=4035d2a1-5584-48cb-89a1-69327c62d415;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567817316047;turbo=0;user-id=53557506;user-type=mod :notanothertaco!notanothertaco@notanothertaco.tmi.twitch.tv PRIVMSG #pajlada :Subway1500 bonus150 later Puhdader, have a great birthday weekend muh d00d. dadoHeart respawnH deluxe4Love echoLove)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/46;badges=subscriber/36,premium/1;bits=101;color=#CC0000;display-name=XFactor709;emotes=35919:31-39;flags=22-29:A.3;id=74f6bfed-3954-46d7-b6f9-f3897857d353;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567815309039;turbo=0;user-id=44386694;user-type= :xfactor709!xfactor709@xfactor709.tmi.twitch.tv PRIVMSG #pajlada :dadocheer101 HA HA HA YOU SUCK dadoKappa)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/46;badges=subscriber/36,premium/1;bits=201;color=#CC0000;display-name=XFactor709;emotes=35919:27-35;flags=13-21:P.5;id=d1d3f022-cd58-4aaf-93c6-b0a119c7e603;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567815144895;turbo=0;user-id=44386694;user-type= :xfactor709!xfactor709@xfactor709.tmi.twitch.tv PRIVMSG #pajlada :dadocheer201 screw you dad dadoKappa)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=300;color=#FF4500;display-name=TheCosmicMonkey;emotes=;flags=;id=0628bc2e-54df-42b9-9dbb-3f0b1bf99df4;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567815042946;turbo=0;user-id=89252569;user-type= :thecosmicmonkey!thecosmicmonkey@thecosmicmonkey.tmi.twitch.tv PRIVMSG #pajlada :ShowLove100 ShowLove100 ShowLove100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,premium/1;bits=250;color=#FFA500;display-name=CicloopeVesgo;emotes=;flags=;id=97ebeaab-5c10-45b7-ae26-2d705ab83c7f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567810126619;turbo=0;user-id=190550415;user-type= :cicloopevesgo!cicloopevesgo@cicloopevesgo.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood250 TAMO BÃO SIM FRENÃO.. vlws por perguntar, respondi la aeheheahhe)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/2;bits=5;color=#FF4500;display-name=The_juno;emotes=;flags=;id=283dd0b3-ec8c-41de-a163-f3c9090636c6;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567879060795;turbo=0;user-id=225124588;user-type= :the_juno!the_juno@the_juno.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1 ShowLove1 ShowLove1 ShowLove1 ShowLove1 beat zaldore don't tell him i am here)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/11;badges=subscriber/6,sub-gifter/5;bits=500;color=;display-name=AcidDensity;emotes=965738:31-38/426170:40-50;flags=;id=5604dec4-e346-4e52-82e0-37addcde7706;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567798945729;turbo=0;user-id=74499402;user-type= :aciddensity!aciddensity@aciddensity.tmi.twitch.tv PRIVMSG #pajlada :Party500 Happy Birthday Piper! PartyHat PartyPopper Hope this year is even better than the last!)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=;bits=222;color=#0000FF;display-name=JamesDaywalker;emotes=;flags=;id=8f68b570-d1c6-47dc-9395-09bd8cd47a85;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567630562967;turbo=0;user-id=65273208;user-type= :jamesdaywalker!jamesdaywalker@jamesdaywalker.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Good afternoon Shy and rage how are you doing this afternoon Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Scoops1 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/33;badges=subscriber/24,bits-leader/3;bits=4;color=#FF0000;display-name=TeamNova_Gaming;emotes=;flags=;id=dcf33211-70ac-4d8e-b94e-593dd5173872;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567626417620;turbo=0;user-id=68998030;user-type= :teamnova_gaming!teamnova_gaming@teamnova_gaming.tmi.twitch.tv PRIVMSG #pajlada :hahCheer1 hahCheer1 hahCheer1 hahCheer1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/33;badges=subscriber/24,bits/1000;bits=1;color=#FF0000;display-name=TeamNova_Gaming;emotes=;flags=;id=9e43d830-b073-41f2-bd71-99398381d391;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567626370374;turbo=0;user-id=68998030;user-type= :teamnova_gaming!teamnova_gaming@teamnova_gaming.tmi.twitch.tv PRIVMSG #pajlada :Shamrock1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=20;color=;display-name=BeastieBoy1604;emotes=;flags=;id=901575ce-8dbb-42cf-9269-31ad890c2613;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567801006800;turbo=0;user-id=183378449;user-type= :beastieboy1604!beastieboy1604@beastieboy1604.tmi.twitch.tv PRIVMSG #pajlada :uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1 uni1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=3;color=#7604E7;display-name=L0u_uRu;emotes=;flags=;id=838df663-67dc-4bbd-9f97-5ebbd3f1a2f6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567274285780;turbo=0;user-id=221466968;user-type= :l0u_uru!l0u_uru@l0u_uru.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=;display-name=node_p;emotes=;flags=;id=7ffc3c62-6362-4827-8f76-65a9af01634a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567203012996;turbo=0;user-id=177670314;user-type= :node_p!node_p@node_p.tmi.twitch.tv PRIVMSG #pajlada :VoHiYo1 hello)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/8;badges=subscriber/6,bits-leader/1;bits=200;color=#FF0000;display-name=blazingplague;emotes=;flags=;id=d3dfe07c-193d-48f9-8ce8-a2cb60239c51;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567820623121;turbo=0;user-id=247189263;user-type= :blazingplague!blazingplague@blazingplague.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 bonus20)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/8;badges=subscriber/6,bits-leader/2;bits=400;color=#FF0000;display-name=blazingplague;emotes=;flags=;id=5c85be56-db0e-4d3f-85a2-b0b232c52f5e;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567820571166;turbo=0;user-id=247189263;user-type= :blazingplague!blazingplague@blazingplague.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 Subway100 Subway100 bonus40)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=500;color=;display-name=xbrutalxutex;emotes=;flags=;id=67e8095e-36fe-4441-86e4-41004be8c252;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567820505737;turbo=0;user-id=61857703;user-type= :xbrutalxutex!xbrutalxutex@xbrutalxutex.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Cheer100 Cheer100 Cheer100 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1000;bits=1000;color=;display-name=mrchis;emotes=;flags=;id=d3681cfc-1c1c-42f1-85b8-911c8788d3a7;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567843020604;turbo=0;user-id=460201335;user-type= :mrchis!mrchis@mrchis.tmi.twitch.tv PRIVMSG #pajlada :Keep her awake! PogChamp1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/2;badges=subscriber/0,bits/1000;bits=200;color=#FF69B4;display-name=7AdorableBrian7;emotes=;flags=;id=0d7730ac-9da0-405d-a51c-c7279707324f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567235164992;turbo=0;user-id=77034309;user-type= :7adorablebrian7!7adorablebrian7@7adorablebrian7.tmi.twitch.tv PRIVMSG #pajlada :Scoops100 lanaeCheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/32;badges=subscriber/30,bits/10000;bits=5;color=#0000FF;display-name=Terrytoontwitch;emotes=;flags=;id=03583feb-c97e-469e-a476-b759af818e9c;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567234127669;turbo=0;user-id=84683122;user-type= :terrytoontwitch!terrytoontwitch@terrytoontwitch.tmi.twitch.tv PRIVMSG #pajlada :lanaeCheer1 lanaeCheer1 lanaeCheer1 lanaeCheer1 lanaeCheer1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/32;badges=subscriber/30,bits/10000;bits=5;color=#0000FF;display-name=Terrytoontwitch;emotes=;flags=;id=8b3b0bee-81ab-4cb9-be99-7a4c7dab477d;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567233489131;turbo=0;user-id=84683122;user-type= :terrytoontwitch!terrytoontwitch@terrytoontwitch.tmi.twitch.tv PRIVMSG #pajlada :lanaeCheer1 lanaeCheer1 lanaeCheer1 lanaeCheer1 lanaeCheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=100;color=;display-name=yosemuhtee;emotes=;flags=;id=67555452-a1d7-48bd-af74-42cbfae9b8e4;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567565316776;turbo=0;user-id=232538489;user-type= :yosemuhtee!yosemuhtee@yosemuhtee.tmi.twitch.tv PRIVMSG #pajlada :pride100 WOOOOOOOOOOOOOOOOOOOOO KEEP GAMING GAMER WOOOOOOOOOOOOOOOOOOOOOOOOOOO)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=125;color=;display-name=yosemuhtee;emotes=;flags=;id=3b33b11b-05be-47a4-ad24-46c4c6830aa0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567467071767;turbo=0;user-id=232538489;user-type= :yosemuhtee!yosemuhtee@yosemuhtee.tmi.twitch.tv PRIVMSG #pajlada :pride125 steezy eeezy eezy eezy WOOOOOOOOOOOO WOOOOOOOOOOO WOOOOOOOOOOO WOOOOOOOOOOOO)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=1500;color=#00D2FF;display-name=kyoh_;emotes=;flags=;id=96261376-9256-4538-8b14-2775594e9b82;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567720049209;turbo=0;user-id=126763234;user-type= :kyoh_!kyoh_@kyoh_.tmi.twitch.tv PRIVMSG #pajlada :Subway1000 Subway100 Subway100 Subway100 Subway100 Subway100 bonus150)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=500;color=#00D2FF;display-name=kyoh_;emotes=;flags=;id=7fadb649-9b12-47ef-be84-57d5e08a3849;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567719612973;turbo=0;user-id=126763234;user-type= :kyoh_!kyoh_@kyoh_.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 Subway100 Subway100 Subway100 bonus50)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=500;color=;display-name=thisisunagi;emotes=;flags=;id=761155d0-e7b4-4376-8726-fb8170737028;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567803005878;turbo=0;user-id=236185606;user-type= :thisisunagi!thisisunagi@thisisunagi.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Cheer100 Cheer100 Cheer100 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=subscriber/3,bits/10000;bits=100;color=;display-name=kingterrapin;emotes=;flags=;id=f01a8034-dfad-4c7d-b9f3-05fbf26abe5f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567802272953;turbo=0;user-id=221451302;user-type= :kingterrapin!kingterrapin@kingterrapin.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 but also tell fruit he’s bad with banjo kazooie)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=subscriber/3,bits/10000;bits=500;color=;display-name=kingterrapin;emotes=;flags=;id=1167f9e3-0979-435c-a112-1ee0a509f5f2;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567802159685;turbo=0;user-id=221451302;user-type= :kingterrapin!kingterrapin@kingterrapin.tmi.twitch.tv PRIVMSG #pajlada :uni100 uni100 uni100 uni100 uni100 I realize I have been talking too much during streams so here’s a simple 500)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=;display-name=mugzeatspants;emotes=;flags=;id=55f30c81-7361-4ee7-b3c1-0f5c2284877f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567801066311;turbo=0;user-id=452788601;user-type= :mugzeatspants!mugzeatspants@mugzeatspants.tmi.twitch.tv PRIVMSG #pajlada :Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=;display-name=mugzeatspants;emotes=;flags=;id=9d56dbdb-bfd3-49c6-a585-0daccd587ccd;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567800960379;turbo=0;user-id=452788601;user-type= :mugzeatspants!mugzeatspants@mugzeatspants.tmi.twitch.tv PRIVMSG #pajlada :Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/2;badges=subscriber/0,bits-leader/2;bits=100;color=#2E8B57;display-name=Im_Rastaman420;emotes=1:20-21;flags=;id=00dbb5b6-c500-4585-bd15-69920ecf39eb;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567474685905;turbo=0;user-id=199310387;user-type= :im_rastaman420!im_rastaman420@im_rastaman420.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Bonne nuit :))");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=20;color=#06952D;display-name=Belchavez;emotes=;flags=;id=df270230-4632-44c4-afaf-57ad5e16602c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567784861754;turbo=0;user-id=156668711;user-type= :belchavez!belchavez@belchavez.tmi.twitch.tv PRIVMSG #pajlada :lurkers unite Subway10 Subway10 bonus2)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=50;color=#06952D;display-name=Belchavez;emotes=;flags=;id=491f0ad2-e2f6-4011-9b1d-3ec40277051a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567782238798;turbo=0;user-id=156668711;user-type= :belchavez!belchavez@belchavez.tmi.twitch.tv PRIVMSG #pajlada :Clutch Round! Subway10 Subway10 Subway10 Subway10 Subway10 bonus5)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=#008000;display-name=Droiler;emotes=;flags=;id=cf68dffd-3be8-4248-b15b-a8320568a79e;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567799865891;turbo=0;user-id=124958281;user-type= :droiler!droiler@droiler.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 "UUUuUUuuUuuUUuUU" - Sans)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/15;badges=moderator/1,subscriber/12,partner/1;bits=100;color=#870FFF;display-name=Lowco;emotes=;flags=;id=64e0fea2-e9ae-44d1-b2c3-2738290d964a;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872854492;turbo=0;user-id=52925091;user-type=mod :lowco!lowco@lowco.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/6;badges=subscriber/6,sub-gifter/5;bits=200;color=#FF4500;display-name=KleetusPlays;emotes=;flags=;id=a34d1036-5b36-4cc4-bec5-8757a6f65ca1;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567872815988;turbo=0;user-id=47520800;user-type= :kleetusplays!kleetusplays@kleetusplays.tmi.twitch.tv PRIVMSG #pajlada :Subway200 bonus20)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=subscriber/3,bits/1;bits=28;color=#5F9EA0;display-name=greyhastur;emotes=;flags=;id=76369958-bd94-4d7f-affb-47540d35d1a5;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567706441324;turbo=0;user-id=156171733;user-type= :greyhastur!greyhastur@greyhastur.tmi.twitch.tv PRIVMSG #pajlada :Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 bonus2)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=25;color=#05A3C7;display-name=legitc0;emotes=;flags=;id=adfd5f15-a501-4c42-9ea0-c4dfc0e63560;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567729982330;turbo=0;user-id=121577607;user-type= :legitc0!legitc0@legitc0.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=;display-name=INQ_Nox;emotes=;flags=;id=73989b88-27f9-4b7f-a060-2097ad2a4e3f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567883865306;turbo=0;user-id=443872907;user-type= :inq_nox!inq_nox@inq_nox.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Make Stan 106. Pleaseeeeeee)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/11;badges=subscriber/9,bits/1000;bits=1000;color=;display-name=wyvernness;emotes=;flags=;id=96588ace-71fe-4995-92e1-81b6daf660c8;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567883283379;turbo=0;user-id=249293921;user-type= :wyvernness!wyvernness@wyvernness.tmi.twitch.tv PRIVMSG #pajlada :Subway1000 bonus100 Don't die have some bits)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/16;badges=subscriber/12,bits/5000;bits=100;color=#2C38E2;display-name=Butler063;emotes=;flags=;id=c472544f-b0f0-4aeb-9cf2-6fc0df724270;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567883039094;turbo=0;user-id=31498188;user-type= :butler063!butler063@butler063.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 Remember survive at all costs for Subway!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=subscriber/3;bits=100;color=;display-name=mopsmannen;emotes=;flags=;id=591fdd16-ef08-45fa-ae7c-a8dde6eebe16;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882865997;turbo=0;user-id=86506465;user-type= :mopsmannen!mopsmannen@mopsmannen.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Hewwo dis ish teh FBI, Furry Bulge Inspection agency, coming to awwest u for poswession of an iwwegally big bulgie uwu. Now I will inspwect u. sniffs and notices ur bulge owo wats dis squeezie ur bulgie uwu ish sho juicy and big owo)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=500;color=#00FF7F;display-name=Shevress;emotes=;flags=;id=02d61274-6703-47a2-9985-936fdebabe54;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567882725450;turbo=0;user-id=172068386;user-type= :shevress!shevress@shevress.tmi.twitch.tv PRIVMSG #pajlada :cheer500 A little Birthday gift. Have you reminded your friends, that you have a new birthday?)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=subscriber/3,bits/100;bits=100;color=#CC002F;display-name=TheKr1st0ff;emotes=;flags=;id=453cd6f3-2473-4aed-838e-a95bf1ed890f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882637383;turbo=0;user-id=206755305;user-type= :thekr1st0ff!thekr1st0ff@thekr1st0ff.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 remember to use the subway emote to give 10% more bits)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=400;color=;display-name=malt3525;emotes=;flags=;id=c93e50d9-bfde-4e3d-9738-7cbed58df887;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567882568190;turbo=0;user-id=123798342;user-type= :malt3525!malt3525@malt3525.tmi.twitch.tv PRIVMSG #pajlada :Party100 Party100 Party100 Party100 we know its your birthday kaif stop lying)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/12;badges=subscriber/12,bits/100;bits=100;color=#FF69B4;display-name=omegaV3;emotes=;flags=;id=15f24a5d-c317-4081-ba18-cd8262f607fd;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882549276;turbo=0;user-id=86155006;user-type= :omegav3!omegav3@omegav3.tmi.twitch.tv PRIVMSG #pajlada :Party100 happy birfday kiff daddy)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/2;badges=subscriber/0,sub-gifter/1;bits=100;color=#0000FF;display-name=TimmE1;emotes=;flags=;id=451e1a2b-ef3b-4d88-8b5b-01ba5e3ecb1f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882506215;turbo=0;user-id=439378010;user-type= :timme1!timme1@timme1.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 have a wonderful bday kaifu)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/10000;bits=100;color=#00FF7F;display-name=Shevress;emotes=3:47-48;flags=;id=2a9b7463-0711-4c3c-9ffb-e8f950081671;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567882475711;turbo=0;user-id=172068386;user-type= :shevress!shevress@shevress.tmi.twitch.tv PRIVMSG #pajlada :cheer100 Ah i didn't know that. Happy Birthday :D)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1000;bits=30;color=#FF860A;display-name=Legend001;emotes=;flags=;id=5295ba74-b806-4873-8d8c-5586fd780ecc;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567793125293;turbo=0;user-id=144207020;user-type= :legend001!legend001@legend001.tmi.twitch.tv PRIVMSG #pajlada :proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1 proxCheer1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/11;badges=subscriber/6,bits/1000;bits=35;color=#1E90FF;display-name=Darketzu;emotes=;flags=;id=39457378-bbbc-4d9f-966f-88bb151d03ac;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567873446062;turbo=0;user-id=155070276;user-type= :darketzu!darketzu@darketzu.tmi.twitch.tv PRIVMSG #pajlada :Subway35 bonus3)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=subscriber/6,bits-leader/1;bits=14;color=#1E90FF;display-name=SwiceT;emotes=;flags=;id=938a73d5-06df-4941-9249-b67a4a31c685;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567552371588;turbo=0;user-id=71994866;user-type= :swicet!swicet@swicet.tmi.twitch.tv PRIVMSG #pajlada :Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 bonus1 This is the rest of my bits. Happy Subtember)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=subscriber/6,bits-charity/1;bits=10;color=#1E90FF;display-name=SwiceT;emotes=;flags=;id=5647a955-67d2-4480-a4a4-dc6c244e47c3;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567550881565;turbo=0;user-id=71994866;user-type= :swicet!swicet@swicet.tmi.twitch.tv PRIVMSG #pajlada :Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 bonus1 Donate 10 bits and the streamer gets an extra one for free)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/0,bits-leader/1;bits=1500;color=#5F9EA0;display-name=LaurenJW28;emotes=;flags=;id=f439f1df-07c1-474b-a09a-d9ffbd819a7d;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567746208862;turbo=0;user-id=244354979;user-type=mod :laurenjw28!laurenjw28@laurenjw28.tmi.twitch.tv PRIVMSG #pajlada :Cheer1000 Cheer100 Cheer100 Cheer100 Cheer100 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/0,bits/1000;bits=947;color=;display-name=TheAngryTony;emotes=;flags=;id=c369cd98-ad3e-46b4-8df7-9115cbfc1989;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567746158256;turbo=0;user-id=214368626;user-type=mod :theangrytony!theangrytony@theangrytony.tmi.twitch.tv PRIVMSG #pajlada :Subway947 bonus94)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/3;badges=moderator/1,subscriber/0,bits/1000;bits=1500;color=#5F9EA0;display-name=LaurenJW28;emotes=;flags=;id=2403678c-6109-43ac-b3b5-1f5230f91729;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567746107991;turbo=0;user-id=244354979;user-type=mod :laurenjw28!laurenjw28@laurenjw28.tmi.twitch.tv PRIVMSG #pajlada :Cheer1000 Cheer100 Cheer100 Cheer100 Cheer100 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=5;color=#5F9EA0;display-name=drkwings;emotes=;flags=;id=ad45dae5-b985-4526-9b9e-0bdba2d23289;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567742106689;turbo=0;user-id=440230526;user-type= :drkwings!drkwings@drkwings.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1 SeemsGood1 SeemsGood1 SeemsGood1 SeemsGood1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/16;badges=subscriber/12,bits/1000;bits=1;color=;display-name=mustangbugatti;emotes=;flags=;id=ee987ee9-46a4-4c06-bf66-2cafff5d4cdd;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567883658780;turbo=0;user-id=115948494;user-type= :mustangbugatti!mustangbugatti@mustangbugatti.tmi.twitch.tv PRIVMSG #pajlada :(In clarkson accent) Some say...the only number in his contacts is himself..... And...that he is the international butt-dial champion... All we know is.... HES CALLED THE STIG Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/2;badges=subscriber/0,bits/1000;bits=1;color=;display-name=derpysaurus1;emotes=;flags=;id=c41c3d8b-c591-4db0-87e7-a78c5536de82;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567883655116;turbo=0;user-id=419221818;user-type= :derpysaurus1!derpysaurus1@derpysaurus1.tmi.twitch.tv PRIVMSG #pajlada :cheer1 OMG ur back yaaaaaaaaaaaaaaaaaaaaayyyyyyyyy)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=subscriber/0,premium/1;bits=1;color=#8A2BE2;display-name=sirlordstallion;emotes=;flags=;id=61a87aeb-88b1-42f9-90f5-74429d8bf387;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882978939;turbo=0;user-id=92145441;user-type= :sirlordstallion!sirlordstallion@sirlordstallion.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Alex is definetly not putting his eggs in Narreths basket)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=;display-name=xplosivegingerx;emotes=;flags=;id=9e3bd7e1-6758-45d6-8f48-b437a4b4fa29;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882527289;turbo=0;user-id=151265906;user-type= :xplosivegingerx!xplosivegingerx@xplosivegingerx.tmi.twitch.tv PRIVMSG #pajlada :Pride100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1;bits=1;color=;display-name=xplosivegingerx;emotes=;flags=;id=f8aac1e0-050a-44bf-abcc-c0cf12cbedfc;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882249072;turbo=0;user-id=151265906;user-type= :xplosivegingerx!xplosivegingerx@xplosivegingerx.tmi.twitch.tv PRIVMSG #pajlada :Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/17;badges=subscriber/12,bits/10000;bits=150;color=#B22222;display-name=DogsHaveNoUncles;emotes=;flags=;id=4125bb6e-e229-4e7f-b218-1aa6ed690ef9;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567882027546;turbo=0;user-id=41772497;user-type= :dogshavenouncles!dogshavenouncles@dogshavenouncles.tmi.twitch.tv PRIVMSG #pajlada :cheer150 I used to make origami, but I stopped as it was too much paperwork)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=500;color=;display-name=AlexJohanning;emotes=;flags=;id=4e4229a3-e7f2-4082-8c55-47d42db3b09c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567881969862;turbo=0;user-id=190390930;user-type= :alexjohanning!alexjohanning@alexjohanning.tmi.twitch.tv PRIVMSG #pajlada :cheer500)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=245;color=;display-name=undonebunion6;emotes=;flags=;id=331ec583-0a80-4299-9206-0efd9e33d934;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567881553759;turbo=0;user-id=452974274;user-type= :undonebunion6!undonebunion6@undonebunion6.tmi.twitch.tv PRIVMSG #pajlada :cheer245 can I join?)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=;display-name=therealruffnix;emotes=;flags=61-67:S.6;id=25f567ad-ac95-45ab-b12e-4d647f6a2345;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567524218162;turbo=0;user-id=55059620;user-type= :therealruffnix!therealruffnix@therealruffnix.tmi.twitch.tv PRIVMSG #pajlada :cheer100 This is the kind of ASMR I'm missing on YouTube and PornHub)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=;display-name=BeamMeUpSnotty;emotes=;flags=;id=8022f41f-dcb8-42f2-b46a-04d4a99180bd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567270037926;turbo=0;user-id=261679182;user-type= :beammeupsnotty!beammeupsnotty@beammeupsnotty.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#00FF7F;display-name=EXDE_HUN;emotes=;flags=;id=60d8835b-23fa-418c-96ca-5874e5d5e8ba;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566654664248;turbo=0;user-id=129793695;user-type= :exde_hun!exde_hun@exde_hun.tmi.twitch.tv PRIVMSG #pajlada :PogChamp10)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=5;color=;display-name=slyckity;emotes=;flags=;id=fd6c5507-3a4e-4d24-8f6e-fadf07f520d3;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824273752;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=5;color=;display-name=slyckity;emotes=;flags=;id=7003f119-b9a6-4319-a1e8-8e99f96ab01a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824186437;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=10;color=;display-name=slyckity;emotes=;flags=;id=3f7de686-77f6-46d2-919e-404312c6676f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824128736;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=10;color=;display-name=slyckity;emotes=;flags=;id=9e830ed3-8735-4ccb-9a8b-80466598ca19;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824118921;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=5;color=;display-name=slyckity;emotes=;flags=;id=749285d7-6709-407d-ac9b-2cdc553cf12b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567824070261;turbo=0;user-id=143114011;user-type= :slyckity!slyckity@slyckity.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=500;color=#0000FF;display-name=Membah666;emotes=;flags=;id=bedf7599-06cb-4ac4-a6f6-6d4b0cfe89d2;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567654368760;turbo=0;user-id=229332314;user-type= :membah666!membah666@membah666.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Cheer100 Cheer100 Cheer100 Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=#8A2BE2;display-name=Mitchfynde;emotes=;flags=;id=8c9ed7c5-80db-4ebd-9218-5fa75e16ea32;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567654319590;turbo=0;user-id=65694832;user-type= :mitchfynde!mitchfynde@mitchfynde.tmi.twitch.tv PRIVMSG #pajlada :Subway100 bonus10 you are the best streamer on earth, monkey!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/9;badges=moderator/1,subscriber/9,premium/1;bits=10;color=#FF0000;display-name=alisonjimmy;emotes=;flags=;id=c3094a7f-9d73-400d-8ba7-da1330441d07;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567874361702;turbo=0;user-id=32117492;user-type=mod :alisonjimmy!alisonjimmy@alisonjimmy.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 Aproveitando o tema de floresta. Faz o urro do Tarzan?)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=861b0c3c-9f2c-44e8-a7b8-067b46862b37;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567874333913;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 faz o urro? pçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=a288eb4a-bc83-481e-ada9-27d37c6bd656;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872740628;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 oi. eu sou o bot nóia. açustar)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=a8e0522c-2b77-4ee1-b60e-47a4734fcb03;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872703649;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 assustar)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=940654e2-0fbd-4fa3-9991-81898ff93faf;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872674978;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 açustar)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/9;badges=moderator/1,subscriber/9,premium/1;bits=10;color=#FF0000;display-name=alisonjimmy;emotes=;flags=;id=d8009b18-5912-44c5-afee-f511553fd6c4;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872671649;turbo=0;user-id=32117492;user-type=mod :alisonjimmy!alisonjimmy@alisonjimmy.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 Aqueles índios era da tribo Gira No Matu. Eles costumam açustar as onças com dialetos antigos como pçpçpçpçpçpçpçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/9;badges=moderator/1,subscriber/9,premium/1;bits=10;color=#FF0000;display-name=alisonjimmy;emotes=;flags=;id=f1d5c5ef-fc55-4498-bf6b-22a985d93389;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872615171;turbo=0;user-id=32117492;user-type=mod :alisonjimmy!alisonjimmy@alisonjimmy.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 Bandidinho)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/9;badges=moderator/1,subscriber/9,premium/1;bits=10;color=#FF0000;display-name=alisonjimmy;emotes=;flags=;id=f6bfcada-17a9-4c96-99d3-6409400a0c93;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872586886;turbo=0;user-id=32117492;user-type=mod :alisonjimmy!alisonjimmy@alisonjimmy.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 Aqueles índios era da tribo Gira No Matu. Eles costumam assustar as onças com dialetos antigos como pçpçpçpçpçpçpçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=f3749f4b-b9b6-4736-8cff-3efb07b77d25;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872566198;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 Aqueles índios era da tribo GiraNOMatu. Eles costumam assustar as onças com dialetos antigos como pçpçpçpçpçpçpçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/9;badges=moderator/1,subscriber/9,premium/1;bits=10;color=#FF0000;display-name=alisonjimmy;emotes=;flags=;id=87fbb800-8de2-479e-9fba-a68b84108b69;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872526929;turbo=0;user-id=32117492;user-type=mod :alisonjimmy!alisonjimmy@alisonjimmy.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 Aqueles índios era da tribo Gira Nu Matu. Eles costumam assustar as onças com dialetos antigos como pçpçpçpçpçpçpçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=055d653d-fd3e-4fb7-ba9f-08b34568f7b4;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872477768;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 Aqueles índios era da tribo GiraNuMatu. Eles costumam assustar as onças com dialetos antigos como pçpçpçpçpçpçpçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/9;badges=moderator/1,subscriber/9,premium/1;bits=10;color=#FF0000;display-name=alisonjimmy;emotes=;flags=;id=1e1f4921-2515-40b5-aa80-a0ffb2bcdf93;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567872414466;turbo=0;user-id=32117492;user-type=mod :alisonjimmy!alisonjimmy@alisonjimmy.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 Aqueles índios era da tribo GiraNuMatu. Eles costumam assustar as onças com dialetos antigos como pçpçpçpçpçpçpçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,premium/1;bits=10;color=#00FF7F;display-name=collacobh;emotes=;flags=;id=a1b3ed7c-a4c4-4deb-a9dd-db8a3b35d4ca;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567871415398;turbo=0;user-id=120527136;user-type=mod :collacobh!collacobh@collacobh.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 setembrochove? pçpçpçpçpçpçpçpç)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/1000;bits=200;color=#15A1B7;display-name=Threemoons;emotes=;flags=;id=450181d4-8a06-4aa9-8918-e97b7f2a7c3d;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567828290783;turbo=0;user-id=38654319;user-type= :threemoons!threemoons@threemoons.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 bonus20)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/6;badges=subscriber/6,bits/100;bits=6;color=;display-name=BlaqkAssassin;emotes=9:54-55;flags=38-52:A.6/P.6;id=2d86a957-c9c6-48d2-93a1-841023e0709b;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567825820673;turbo=0;user-id=74919189;user-type= :blaqkassassin!blaqkassassin@blaqkassassin.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1 ShowLove1 Pride1 You're all fucking weirdos <3 Pride1 ShowLove1 ShowLove1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/6;badges=subscriber/6,bits/100;bits=300;color=;display-name=BlaqkAssassin;emotes=;flags=9-13:P.6/S.6;id=e20520d6-eeba-47eb-bd94-6f913a37b6b0;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567824680492;turbo=0;user-id=74919189;user-type= :blaqkassassin!blaqkassassin@blaqkassassin.tmi.twitch.tv PRIVMSG #pajlada :Pride100 *Jizz on dem holes* Pride100 Pride100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=#FF4500;display-name=Shenkie;emotes=;flags=;id=7d1bab84-092a-4bf0-98d4-cc8e0a804bd2;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567835079931;turbo=0;user-id=205080502;user-type= :shenkie!shenkie@shenkie.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Lets GO BOYS!!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=subscriber/3,bits/100;bits=100;color=#FF69B4;display-name=FlipDoodleRama;emotes=;flags=;id=09681de2-af9b-4a21-bb68-660371f14ce3;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567834145037;turbo=0;user-id=61317728;user-type= :flipdoodlerama!flipdoodlerama@flipdoodlerama.tmi.twitch.tv PRIVMSG #pajlada :ronnoCheer100 do me a small favor, and ask EVScape to spell "ICUP")");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=41;color=#8A2BE2;display-name=derbockwursttoeter;emotes=;flags=;id=80f38066-0ff8-4c2c-80b7-e3edf9628215;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567813882146;turbo=0;user-id=148764459;user-type= :derbockwursttoeter!derbockwursttoeter@derbockwursttoeter.tmi.twitch.tv PRIVMSG #pajlada :Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 Subway1 bonus4)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=64b4f2ab-fe0d-4115-b1c2-b7cb4237ff9a;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567864725690;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :Subway1000 Subway1000 bonus200)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=e2a05c63-e826-445a-8a54-637924325f1c;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567596363583;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :Subway1000 Subway1000 bonus200)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=a823fc81-a742-477a-b8d7-fb590583a305;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567430682996;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1000 ShowLove1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=afecf92d-ecad-48c0-9181-75bc444f0724;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567181225161;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1000 ShowLove1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=f96d663e-f56e-46fc-8dbf-b11ffb1e00cb;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567085138165;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :Scoops1000 Scoops1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=91edd61e-6e8e-488a-baa3-edc794147e0a;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1566998586630;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :Scoops1000 Scoops1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=3e20b7a0-b075-4c3b-a585-125616cbce9f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1566667073706;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1000 ShowLove1000)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=1000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=42e6e7f4-65bc-416d-913a-ac3f0fd310ea;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1566575854968;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1000)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=;display-name=migimimimigimemigimimi;emotes=;flags=;id=e29ce2bb-744e-4388-8981-fdb18f5432eb;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566410262921;turbo=0;user-id=444193558;user-type= :migimimimigimemigimimi!migimimimigimemigimimi@migimimimigimemigimimi.tmi.twitch.tv PRIVMSG #pajlada :Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,premium/1;bits=2000;color=#FF7F50;display-name=michie4017;emotes=;flags=;id=fe22ba01-54f0-45f7-8df0-5e6da02e22e7;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1566405717150;turbo=0;user-id=166936021;user-type= :michie4017!michie4017@michie4017.tmi.twitch.tv PRIVMSG #pajlada :ShowLove1000 ShowLove1000)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=sub-gifter/10;bits=500;color=#6E00FF;display-name=DamAgE0706;emotes=;flags=;id=458bd6ea-bc20-4d96-a0a0-41bc763b9661;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567527610664;turbo=0;user-id=175991251;user-type= :damage0706!damage0706@damage0706.tmi.twitch.tv PRIVMSG #pajlada :uni100 uni100 uni100 uni100 uni100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=sub-gifter/10;bits=500;color=#6E00FF;display-name=DamAgE0706;emotes=;flags=;id=ea1d5c4c-9871-4900-beac-377c9ed78423;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567527493429;turbo=0;user-id=175991251;user-type= :damage0706!damage0706@damage0706.tmi.twitch.tv PRIVMSG #pajlada :Party100 Party100 Party100 Party100 Party100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1000;bits=500;color=#0000FF;display-name=LinerGL;emotes=;flags=;id=e6e36c2b-08ba-482e-852d-9205ed81f90f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567809723859;turbo=0;user-id=184109582;user-type= :linergl!linergl@linergl.tmi.twitch.tv PRIVMSG #pajlada :ShowLove100 ShowLove100 ShowLove100 ShowLove100 ShowLove100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/25;badges=subscriber/12,bits-leader/2;bits=1000;color=#1E90FF;display-name=plohishrakan;emotes=;flags=;id=af30e5d1-33ab-4334-8a6e-2075aa9c2548;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567807627792;turbo=0;user-id=105643346;user-type= :plohishrakan!plohishrakan@plohishrakan.tmi.twitch.tv PRIVMSG #pajlada :rizhayaCheer1000 тест)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/25;badges=subscriber/12,bits-leader/2;bits=100;color=#1E90FF;display-name=plohishrakan;emotes=;flags=;id=aefab25d-ac2a-4988-884c-4504691d5583;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567807369475;turbo=0;user-id=105643346;user-type= :plohishrakan!plohishrakan@plohishrakan.tmi.twitch.tv PRIVMSG #pajlada :uni100 ну хоть теперь я намбер ван)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/5;badges=subscriber/3,bits-leader/1;bits=500;color=#5A0B8F;display-name=Mojitox;emotes=;flags=;id=868e20be-030a-475f-82d3-cb7aab0e8dff;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567607882938;turbo=0;user-id=174570715;user-type= :mojitox!mojitox@mojitox.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 Cheer100 Cheer100 Cheer100 Cheer100 yippi da yeah du geiles Ferkel)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,sub-gifter/1;bits=700;color=#0000FF;display-name=aayronc406;emotes=;flags=;id=c6545632-c07c-42c7-97a2-795e9168b9ad;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567317328530;turbo=0;user-id=189446289;user-type= :aayronc406!aayronc406@aayronc406.tmi.twitch.tv PRIVMSG #pajlada :PJSalt700)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,sub-gifter/1;bits=200;color=#0000FF;display-name=aayronc406;emotes=;flags=;id=58c64cec-91e3-4557-b9c1-ecf19853384c;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567317269814;turbo=0;user-id=189446289;user-type= :aayronc406!aayronc406@aayronc406.tmi.twitch.tv PRIVMSG #pajlada :ShowLove200 Rae is cute and that's final. No arguments!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,sub-gifter/1;bits=100;color=#0000FF;display-name=aayronc406;emotes=;flags=;id=e30de64c-4aa2-44ae-9672-3f45efbfaabb;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567317165162;turbo=0;user-id=189446289;user-type= :aayronc406!aayronc406@aayronc406.tmi.twitch.tv PRIVMSG #pajlada :ShowLove100 soisoisoisoisoisoisoisoisoisoisoisoisoisoi)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,sub-gifter/1;bits=250;color=#0000FF;display-name=aayronc406;emotes=;flags=;id=9b30df19-f19c-492f-bc1c-c06652a2a50c;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567317068120;turbo=0;user-id=189446289;user-type= :aayronc406!aayronc406@aayronc406.tmi.twitch.tv PRIVMSG #pajlada :Scoops250 wewowewowewowewowewo)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,sub-gifter/1;bits=250;color=#0000FF;display-name=aayronc406;emotes=;flags=;id=c0bff8ab-3c6a-4f7e-b694-c5224005f537;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567316999740;turbo=0;user-id=189446289;user-type= :aayronc406!aayronc406@aayronc406.tmi.twitch.tv PRIVMSG #pajlada :Cheer250 Wooooooo, go Rae!!!)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=50;color=#0000FF;display-name=aayronc406;emotes=;flags=;id=b6b93280-e3f2-4d46-a508-e867a92c8574;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567316859131;turbo=0;user-id=189446289;user-type= :aayronc406!aayronc406@aayronc406.tmi.twitch.tv PRIVMSG #pajlada :FrankerZ50)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1000;bits=1;color=#1E90FF;display-name=Shu_Ouma;emotes=;flags=;id=f08e4c85-0c7b-4a15-8c68-1237a0951806;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567787847687;turbo=0;user-id=43600746;user-type= :shu_ouma!shu_ouma@shu_ouma.tmi.twitch.tv PRIVMSG #pajlada :flyrieCheer1 Se der timeout na Alisa_Ntr humidade ta la no teto)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/7;badges=moderator/1,subscriber/6,bits/10000;bits=1500;color=#1E90FF;display-name=Lagiia;emotes=;flags=;id=41376daf-84e1-4560-a871-a3ba74e68217;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567699462168;turbo=0;user-id=141160506;user-type=mod :lagiia!lagiia@lagiia.tmi.twitch.tv PRIVMSG #pajlada :Subway1000 Subway100 Subway100 Subway100 Subway100 Subway100 bonus150)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/6;badges=moderator/1,subscriber/6,sub-gifter/5;bits=5000;color=#1E90FF;display-name=Lagiia;emotes=;flags=;id=78a5f563-1e13-4551-b895-e1b97e97ba9e;mod=1;room-id=111448817;subscriber=1;tmi-sent-ts=1567279385491;turbo=0;user-id=141160506;user-type=mod :lagiia!lagiia@lagiia.tmi.twitch.tv PRIVMSG #pajlada :ShowLove5000)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/10000;bits=10000;color=;display-name=mathmaru;emotes=;flags=;id=00318434-150c-4a3b-8310-baa82942e7e9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567273637572;turbo=0;user-id=151009486;user-type= :mathmaru!mathmaru@mathmaru.tmi.twitch.tv PRIVMSG #pajlada :cheer10000)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=10;color=#DAA520;display-name=bassreis;emotes=;flags=;id=80850e76-0d76-41f3-a1a8-bfb192b87030;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567542951373;turbo=0;user-id=424160702;user-type= :bassreis!bassreis@bassreis.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=vip/1,bits/100;bits=5;color=;display-name=randomhitboxtv;emotes=;flags=;id=68b2d50e-3889-43aa-aed9-c45a598b8881;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567436309794;turbo=0;user-id=137631125;user-type= :randomhitboxtv!randomhitboxtv@randomhitboxtv.tmi.twitch.tv PRIVMSG #pajlada :FailFish1 FailFish1 FailFish1 FailFish1 FailFish1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=vip/1,bits/100;bits=5;color=;display-name=randomhitboxtv;emotes=;flags=;id=7f24457a-1cb2-4919-a396-5d19485925a2;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567436295936;turbo=0;user-id=137631125;user-type= :randomhitboxtv!randomhitboxtv@randomhitboxtv.tmi.twitch.tv PRIVMSG #pajlada :NotLikeThis1 NotLikeThis1 NotLikeThis1 NotLikeThis1 NotLikeThis1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=moderator/1,sub-gifter/1;bits=200;color=#1E90FF;display-name=AstralbIades;emotes=;flags=;id=16c50b4c-a026-4e74-a29a-1330641fefb2;mod=1;room-id=111448817;subscriber=0;tmi-sent-ts=1566767598869;turbo=0;user-id=84675162;user-type=mod :astralbiades!astralbiades@astralbiades.tmi.twitch.tv PRIVMSG #pajlada :Scoops200 FOOD EMOTES 4 ALL)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=#0000FF;display-name=Gorian4;emotes=;flags=;id=091ee7d4-5c3f-4ac5-abbe-7b9122bd9cf7;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1566751745419;turbo=0;user-id=250580550;user-type= :gorian4!gorian4@gorian4.tmi.twitch.tv PRIVMSG #pajlada :I'm blacking out goodnight Scoops1)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=40;color=#FF69B4;display-name=Kamikazekitties;emotes=;flags=;id=5b6dd9a1-811b-4f21-a904-091739781777;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567742001750;turbo=0;user-id=66931792;user-type= :kamikazekitties!kamikazekitties@kamikazekitties.tmi.twitch.tv PRIVMSG #pajlada :Cheer40)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#1E90FF;display-name=Jedted;emotes=;flags=;id=35ae125a-bdfb-4325-8051-91d34e0252f4;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567741902320;turbo=0;user-id=7508985;user-type= :jedted!jedted@jedted.tmi.twitch.tv PRIVMSG #pajlada :Cheer10 fly)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=50;color=#FF69B4;display-name=Kamikazekitties;emotes=;flags=;id=d21e66b3-98f8-4aed-9252-7476ab42fae8;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567741761493;turbo=0;user-id=66931792;user-type= :kamikazekitties!kamikazekitties@kamikazekitties.tmi.twitch.tv PRIVMSG #pajlada :Cheer50)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits-leader/1;bits=10;color=#FF69B4;display-name=Kamikazekitties;emotes=;flags=;id=bfcde0bd-8676-4dca-a72a-667ca302ee36;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567740141661;turbo=0;user-id=66931792;user-type= :kamikazekitties!kamikazekitties@kamikazekitties.tmi.twitch.tv PRIVMSG #pajlada :Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Cheer1 Rope)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=#FF69B4;display-name=Kamikazekitties;emotes=;flags=;id=96966f5f-5d84-4ed0-a329-8e95e2556b94;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567739984178;turbo=0;user-id=66931792;user-type= :kamikazekitties!kamikazekitties@kamikazekitties.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 shark)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-charity/1;bits=1000;color=#FF0000;display-name=Earo;emotes=30259:10-16;flags=;id=46795df1-9d3a-4a80-9153-85241800607f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567257657372;turbo=0;user-id=37003428;user-type= :earo!earo@earo.tmi.twitch.tv PRIVMSG #pajlada :cheer1000 HeyGuys)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=15;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=ab5ec720-5ee1-4fd9-8af1-dc018db0cc31;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440950670;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :ShowLove15)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=377;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=262f4d54-9b21-4f13-aac3-6d3b1051282f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440897074;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :NotLikeThis377)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=233;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=a21aa3b3-1165-49f6-b8ad-d8a8105be4af;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440877405;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :Shamrock233)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=144;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=3556e0ad-b5f8-4190-9c4c-e39c1940d191;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440861545;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :bday144)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=89;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=96e380a5-786d-44b8-819a-529b6adb06ac;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440848361;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :SwiftRage89)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=55;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=79eefe36-32a5-410f-877d-e41a6aff93e4;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440836547;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :RIPCheer55)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=34;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=76239011-65fa-4f6a-a6d6-dc5d5dcbd674;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440816630;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :MrDestructoid34)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=21;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=4c05c97c-7b6c-4ae9-bc91-04e98240c1d5;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440806389;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :TriHard21)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=13;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=755193aa-22b2-43c2-8e11-c1959e850ff5;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440788895;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :FrankerZ13)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=8;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=3b2ecce7-842e-429e-b6c8-9456c4646362;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440774009;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :EleGiggle8)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=5;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=3b8736d1-832d-4152-832a-50c526714fd1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440762580;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :uni5)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=3;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=c13a1540-2a03-4c7d-af50-cb20ed88cefd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440750103;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :Party3)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=2;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=5d889eeb-b6b9-4a4e-91ff-0aecdf297edd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440738337;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :ShowLove2)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=#00FF7F;display-name=Baekjoon;emotes=;flags=;id=da47f91a-40d3-4209-ba1c-0219d8b8ecaf;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567440720363;turbo=0;user-id=73587716;user-type= :baekjoon!baekjoon@baekjoon.tmi.twitch.tv PRIVMSG #pajlada :Scoops1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=10;color=#8A2BE2;display-name=EkimSky;emotes=;flags=;id=8adea5b4-7430-44ea-a666-5ebaceb69441;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567833047623;turbo=0;user-id=42132818;user-type= :ekimsky!ekimsky@ekimsky.tmi.twitch.tv PRIVMSG #pajlada :Hi Cheer10)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=500;color=;display-name=godkiller76;emotes=;flags=;id=80e86bcc-d048-44f3-8073-9a1014568e0c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567753685704;turbo=0;user-id=258838478;user-type= :godkiller76!godkiller76@godkiller76.tmi.twitch.tv PRIVMSG #pajlada :Party100 Party100 Party100 Party100 Party100)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=;display-name=godkiller76;emotes=;flags=;id=a433bcd6-ca0f-4cf2-80a5-e7a707c326a0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567753624330;turbo=0;user-id=258838478;user-type= :godkiller76!godkiller76@godkiller76.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 who is better Logan Paul or ksi get it right I donate more bits)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/8;badges=subscriber/6,premium/1;bits=1500;color=;display-name=chipndale19;emotes=300546347:0-12/300563212:14-24;flags=;id=e3704a3b-9318-48d1-a3fd-fb3d005f359c;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567721720581;turbo=0;user-id=409535990;user-type= :chipndale19!chipndale19@chipndale19.tmi.twitch.tv PRIVMSG #pajlada :americ46Weird americ46Ftt cheer1500)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/100;bits=100;color=;display-name=DamianExplores;emotes=;flags=;id=24a0b2e7-0174-4102-bae7-5bc911e672ca;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567721146904;turbo=0;user-id=190450922;user-type= :damianexplores!damianexplores@damianexplores.tmi.twitch.tv PRIVMSG #pajlada :Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0,bits/100;bits=100;color=;display-name=szeththeassassin;emotes=;flags=;id=55fa71d7-dd06-421a-9265-968ec4ac1e2f;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567841062581;turbo=0;user-id=455887394;user-type= :szeththeassassin!szeththeassassin@szeththeassassin.tmi.twitch.tv PRIVMSG #pajlada :Cheer100)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/2;badges=subscriber/0;bits=5;color=#FF0000;display-name=Nosliw9;emotes=;flags=;id=6a029a36-bbe6-4f47-b715-2acf592db6af;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567673983609;turbo=0;user-id=43213660;user-type= :nosliw9!nosliw9@nosliw9.tmi.twitch.tv PRIVMSG #pajlada :PogChamp5 penny)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/13;badges=subscriber/12,sub-gifter/50;bits=5000;color=;display-name=D74D;emotes=;flags=;id=0517dcba-7213-42cd-8595-64e02b564938;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567841279187;turbo=0;user-id=101466185;user-type= :d74d!d74d@d74d.tmi.twitch.tv PRIVMSG #pajlada :Subway5000 bonus500 you are our Trash)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/4;badges=subscriber/3,bits/5000;bits=500;color=#298BA3;display-name=klyngster;emotes=;flags=;id=9d3c3335-1131-45a4-bc41-c0b116c7e653;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567881131141;turbo=0;user-id=43958892;user-type= :klyngster!klyngster@klyngster.tmi.twitch.tv PRIVMSG #pajlada :Subway100 Subway100 Subway100 Subway100 Subway100 bonus50 Can't get enough of gnoll love, baby!)");
    cheerMessages.emplace_back(R"(@badge-info=subscriber/12;badges=vip/1,subscriber/12,bits-leader/3;bits=7;color=#B22222;display-name=FarFromCasual;emotes=;flags=;id=2e9b5992-c223-42f2-bb8a-f7023f013da6;mod=0;room-id=111448817;subscriber=1;tmi-sent-ts=1567742991909;turbo=0;user-id=93295368;user-type= :farfromcasual!farfromcasual@farfromcasual.tmi.twitch.tv PRIVMSG #pajlada :SwiftRage1 SwiftRage1 SwiftRage1 SwiftRage1 SwiftRage1 SwiftRage1 SwiftRage1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=2c114dec-e6d5-4638-b7b8-31965802bfed;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615872986;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=4ac80853-745b-4b41-97f4-645758e96f35;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615866840;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=8d44bd51-6277-4d28-a446-b5269e907828;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615858789;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=aa3c148c-a40c-4ed2-bbc3-47b31be756e6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615852288;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=6fe351d7-9591-422e-845b-703f9c437a23;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615846582;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=a1e7bfeb-64cd-434c-aefb-36ef103396a8;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615841494;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=2f5341e9-4322-4495-8817-ae7795c375e1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615836213;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=f19dc197-6864-41a0-8083-6c4e09f612cc;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615831535;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=58239ff3-63e3-4265-9003-f0ba2f52acba;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615803845;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=c63770d0-7007-41d2-9d3f-96808b546403;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615799227;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=3b13ba64-22e7-4c0f-af7d-331d3752a45a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615794810;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=1d151a2e-a98e-4feb-946b-1d01fb883980;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615790136;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=f28e6952-9082-48dc-9c8e-c63f9dd1ef9a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615786178;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=7b896887-1764-4dec-9ffc-c1dad7da6273;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615781318;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=21a482d9-136e-4a9f-bc0f-7ead3d906b42;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615776863;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=777e3992-f81c-4f8f-952e-a897250ec863;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615771948;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=ea6f5df0-313b-440f-8277-93e64113851b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615767649;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=1784d3c3-8189-4fac-8ec6-81d9a09157c1;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615765997;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=88f2e2b7-c46d-4c72-ae4a-6df4e1733fab;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615763860;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=c94990b2-f347-4a16-8eef-12f4cb3fde97;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615762767;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=f9299f96-e110-4667-9362-ee6ee1081f7c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615761057;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=2bd97fbd-8b1a-4ac6-a957-d776a24e2936;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615759147;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=5d362b94-5b95-49c6-b985-8648f7016d37;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615757603;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=63f43ff4-0800-4bdd-9082-6692c38a0404;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615756502;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=22f9ec7c-2a59-41e3-9ed5-cb900369a6dd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615754711;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=a64483e7-e03f-4bed-91a9-ae82c4911b8b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615753404;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=4f763aeb-0c35-4c8a-a362-ff2142d2d0f6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615753070;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=373aaaa8-f764-43b2-93a9-725ad1cd2260;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615751350;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=a9feb040-2cb1-4c54-a8c4-1d046886c9c8;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615749724;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=42841770-b162-41e2-bb51-dc91e7892f8f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615747895;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=59a3754e-a73b-431d-a883-fb7331ac2bd6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615747127;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=2a6d77e5-09c7-4eb0-ac6c-cc0374bf47b9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615742171;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=2a849040-1986-442d-a9ce-c8740b87f5b0;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615737663;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :Cheer1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=4b926881-3d75-40f1-a2e2-d2997f8b0a86;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615732556;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=eb9c2272-67fc-4be9-bcf6-705d1aaabd53;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615728272;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=d771aec0-b567-4d92-a589-17f4e9eeb3dc;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615722840;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=145953a8-17f9-4c98-ab28-54d6f6bcbe1d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615718868;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=21369bc6-44d8-4e89-9182-7970af0a3a4d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615717024;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=f9ae0ae9-22af-41fc-b0a0-d99fc919711d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615715093;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=55560296-9fba-4420-a1a4-86b28cfd1814;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615712270;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=5d7ef577-9d1f-4aff-9cbe-cfdeea66f809;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615710584;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=b8cf1032-2909-46cd-b5c3-1e6cd05d9287;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615708940;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=c169a8e4-8e25-44e9-b61a-b1ef491230fd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615707086;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=18dc40ba-e601-4140-ace1-3772376d1f07;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615705259;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=4a30a273-9a7b-4d03-b670-5c672567d290;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615703330;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=fc43dc6d-7707-48e7-a1c4-8bbb53fd4c9a;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615699680;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=7;color=;display-name=Reddie100;emotes=;flags=;id=3c4963db-cef1-4e62-aa33-87f1ecad48c9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615682200;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1 PogChamp1 PogChamp1 PogChamp1 PogChamp1 PogChamp1 PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=c1dc9ab9-9047-4fc5-8410-80a51b1915dc;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615680213;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=adf2d29a-92eb-4e29-af88-2e9b6d79bbf9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615674199;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1 geweldig dit)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=64e93e2e-890c-416c-9b36-4fab7169b5cf;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615669516;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=ba0b1c1c-066b-4fb2-a88a-76b335838032;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615667702;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=3531bd3a-822e-47b5-9678-5acbff528533;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615665827;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=2b654628-2815-4902-858a-533f480bac8d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615663539;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=3b26fe3e-112e-45ce-87c4-f6fc32b05f7b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615663270;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=18e7195c-c6aa-47da-b3ea-47ee1d2b2f91;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615661474;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=01e46a18-a31c-4afd-a1d4-1ebe1e6ded4f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615659590;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=fa0eb45d-09ac-4f1b-bfe7-9472844cf538;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615658197;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=587b034d-dbd9-455d-9172-43f8a4dff466;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615657911;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=6578e9b5-b543-47c8-b7f7-cc316970fe92;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615655967;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=9ff17035-9f5f-485c-aaf8-f8b3d6c9dd60;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615654194;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=3cedc247-f99e-4739-8987-660ca4f3a87f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615652341;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=5d0b1e22-2756-48d7-8e83-e88defe05cbf;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615651954;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=07feafc1-b6b7-4183-9b27-8f162d58e038;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615650216;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=149edebb-a665-4a04-8b84-aa15eea28780;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615648446;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=eaa7bbb3-d156-48a7-ba8a-73bcb2348171;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615646706;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=c3cac916-f8d7-4f2a-9ead-38d393a1a3af;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615646315;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=a870f70a-643c-4574-ba72-1044bc32a69f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615639583;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=6b30d23f-7d9f-471c-b435-bbd3c110f229;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615637363;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=;display-name=Reddie100;emotes=;flags=;id=f461b713-e05a-48e9-89c9-e5a2ed77f712;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615593445;turbo=0;user-id=90270577;user-type= :reddie100!reddie100@reddie100.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=5;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=7a238762-413e-481f-b6a9-0e40505b4359;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615571879;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :Ja @gdrever Kappa5)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=4;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=afad96cb-26d6-46e3-8394-21a20582f31c;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615331308;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp4)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=5;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=480a4756-cbe2-476e-90e5-d3c114047d7b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615326958;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp5)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=e22994b9-b2b7-4543-8e59-3c963458a7c3;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615313384;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=3;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=1755d12b-6cbb-4c75-8bc6-d7c0d1595caf;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615308752;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp3)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=2;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=d29c7ace-f1df-4981-a83e-fb14c4dee7f6;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615304967;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp2)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=0a7dae47-909f-46bd-92c1-cb45305bb337;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615301286;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=4436922f-7398-433d-a81e-0c2c9c2bc81b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615299385;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=4be7f989-7aee-43a1-ba01-9fdf232c9bae;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615297531;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=eca4ce9b-5598-4b98-b71c-7c0b5d02ebff;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615295618;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=2;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=91f53fa0-2620-48df-94fd-bfeb468ee0d9;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615173650;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1 nu 11 Kappa1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=34e52411-99ee-4db5-a568-7d0fc11bad58;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615134404;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=211943f2-7741-467d-b3f8-d1ca326bd25e;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615132714;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=313a9ebe-5973-40f1-bfd9-929a6bced5aa;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615131234;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=a4cfe184-b00f-40cc-bd2b-c31b93361d1f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615129476;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=65c41dfa-496d-4aa9-b7a8-26bd01635dcd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615006963;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=ad70f8c9-4090-4272-8782-f6a3e2f61b93;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615005124;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=d9e8f083-c517-42a9-b191-2ba8d2a4bbdd;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567615003177;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=2d07b8c3-3921-4841-822e-01ae9599f62d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567614969477;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :SeemsGood1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits/1;bits=1;color=#FF0000;display-name=NothingLikeDave;emotes=;flags=;id=baceebb0-b0bb-49cc-bbc0-afab0e3a4b0d;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567614868575;turbo=0;user-id=122795319;user-type= :nothinglikedave!nothinglikedave@nothinglikedave.tmi.twitch.tv PRIVMSG #pajlada :Scoops1)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/1;bits=80;color=;display-name=vizaftw;emotes=;flags=;id=18b020de-45ac-4e8d-a022-8eff410f0b0f;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567776695514;turbo=0;user-id=192969150;user-type= :vizaftw!vizaftw@vizaftw.tmi.twitch.tv PRIVMSG #pajlada :Party80 GZ MY DUDE)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=87;color=;display-name=NWAhenkan123;emotes=;flags=31-39:A.3/P.5;id=c7730465-e904-4206-8e1b-4827c1673398;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567708755175;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :Subway87 bonus8 I'M NUMBER ONE B I T C H)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=1;color=;display-name=NWAhenkan123;emotes=;flags=;id=392f85ff-ec69-4b21-b41a-7de943a6f90e;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567708505033;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :uni1 hahahaha you are such a pvp god)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/2;bits=10;color=;display-name=NWAhenkan123;emotes=;flags=;id=d57ea319-894c-43e4-9887-0bcc574bdf7b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567708457534;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 KILL HIM)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=10;color=;display-name=NWAhenkan123;emotes=;flags=;id=8ba51822-e7df-4f2b-8e8a-1c8a1bca172b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567708401538;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :Subway10 bonus1 what's going on with the server transfer my dude?)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=1;color=;display-name=NWAhenkan123;emotes=;flags=35-45:A.3,56-62:S.5;id=03b7f58b-8596-4f3b-a2fb-52121655bb6b;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567706727257;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :PogChamp1 YES!!! how many bits for you sucking your own d i c k on stream?)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=1;color=;display-name=NWAhenkan123;emotes=;flags=44-50:S.5;id=9205db72-912f-4133-9924-ba4410e24cb8;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567706535848;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :Kappa1 you are 100% trying to suck your own d i c k now. mad respect, hope all goes well.)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=1;color=;display-name=NWAhenkan123;emotes=;flags=36-42:S.5;id=3f2b1e7c-1fe5-4622-ace6-bd9dc427a7f8;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567706201529;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :Pride1 you would 100% suck your own d i c k if you could.)");
    cheerMessages.emplace_back(R"(@badge-info=;badges=bits-leader/3;bits=100;color=;display-name=NWAhenkan123;emotes=;flags=43-49:P.6,51-57:S.5;id=d04f8b05-7c2d-4002-9c34-d5ad4c6bc867;mod=0;room-id=111448817;subscriber=0;tmi-sent-ts=1567705991018;turbo=0;user-id=192969150;user-type= :nwahenkan123!nwahenkan123@nwahenkan123.tmi.twitch.tv PRIVMSG #pajlada :Cheer100 HAHAHAHA you have tasted your own fucking dick!!! you are so sick and i respect it.)");

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

    createWindowShortcut(this, "F7", [=] {
        const auto &messages = cheerMessages;
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch.server->addFakeMessage(msg);
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

    // CTRL + 1-8 to open corresponding tab.
    for (auto i = 0; i < 8; i++)
    {
        char hotkey[7];
        std::sprintf(hotkey, "CTRL+%d", i + 1);
        const auto openTab = [this, i] { this->notebook_->selectIndex(i); };
        createWindowShortcut(this, hotkey, openTab);
    }

    createWindowShortcut(this, "CTRL+9",
                         [this] { this->notebook_->selectLastTab(); });

    createWindowShortcut(this, "CTRL+TAB",
                         [this] { this->notebook_->selectNextTab(); });
    createWindowShortcut(this, "CTRL+SHIFT+TAB",
                         [this] { this->notebook_->selectPreviousTab(); });

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

void Window::addMenuBar()
{
    QMenuBar *mainMenu = new QMenuBar();
    mainMenu->setNativeMenuBar(true);

    // First menu.
    QMenu *menu = mainMenu->addMenu(QString());
    QAction *prefs = menu->addAction(QString());
    prefs->setMenuRole(QAction::PreferencesRole);
    connect(prefs, &QAction::triggered, this,
            [] { SettingsDialog::showDialog(); });

    // Window menu.
    QMenu *windowMenu = mainMenu->addMenu(QString("Window"));

    QAction *nextTab = windowMenu->addAction(QString("Select next tab"));
    nextTab->setShortcuts({QKeySequence("Meta+Tab")});
    connect(nextTab, &QAction::triggered, this,
            [=] { this->notebook_->selectNextTab(); });

    QAction *prevTab = windowMenu->addAction(QString("Select previous tab"));
    prevTab->setShortcuts({QKeySequence("Meta+Shift+Tab")});
    connect(prevTab, &QAction::triggered, this,
            [=] { this->notebook_->selectPreviousTab(); });
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
    auto windowTitleEnd = QString("Chatterino " CHATTERINO_VERSION);
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
