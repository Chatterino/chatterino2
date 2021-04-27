#include "widgets/Window.hpp"

#include "Application.hpp"
#include "common/Credentials.hpp"
#include "common/Modes.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Updates.hpp"
#include "singletons/WindowManager.hpp"
#include "util/InitUpdateButton.hpp"
#include "util/Shortcut.hpp"
#include "widgets/AccountSwitchPopup.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/dialogs/UpdateDialog.hpp"
#include "widgets/dialogs/WelcomeDialog.hpp"
#include "widgets/dialogs/switcher/QuickSwitcherPopup.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/TitlebarButton.hpp"
#include "widgets/splits/ClosedSplits.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#ifdef C_DEBUG
#    include <rapidjson/document.h>
#    include "providers/twitch/PubsubClient.hpp"
#    include "util/SampleCheerMessages.hpp"
#    include "util/SampleLinks.hpp"
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QHeaderView>
#include <QMenuBar>
#include <QPalette>
#include <QShortcut>
#include <QStandardItemModel>
#include <QVBoxLayout>

namespace chatterino {

Window::Window(WindowType type)
    : BaseWindow(BaseWindow::EnableCustomFrame)
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
        getApp()->accounts->twitch.currentUserChanged, [this] {
            this->onAccountSelected();
        });
    this->onAccountSelected();

    if (type == WindowType::Main)
    {
        this->resize(int(600 * this->scale()), int(500 * this->scale()));
        getSettings()->tabDirection.connect([this](int val) {
            this->notebook_->setTabDirection(NotebookTabDirection(val));
        });
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

        case QEvent::WindowDeactivate: {
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
}

void Window::addDebugStuff()
{
#ifdef C_DEBUG
    std::vector<QString> cheerMessages, subMessages, miscMessages, linkMessages,
        emoteTestMessages;

    cheerMessages = getSampleCheerMessage();
    auto validLinks = getValidLinks();
    auto invalidLinks = getInvalidLinks();
    // clang-format off

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
    miscMessages.emplace_back(R"(@badge-info=founder/47;badges=moderator/1,founder/0,premium/1;color=#00FF80;display-name=gempir;emotes=;flags=;id=d4514490-202e-43cb-b429-ef01a9d9c2fe;mod=1;room-id=11148817;subscriber=0;tmi-sent-ts=1575198233854;turbo=0;user-id=77829817;user-type=mod :gempir!gempir@gempir.tmi.twitch.tv PRIVMSG #pajlada :offline chat gachiBASS)");

    // various link tests
    linkMessages.emplace_back(R"(@badge-info=subscriber/48;badges=broadcaster/1,subscriber/36,partner/1;color=#CC44FF;display-name=pajlada;emotes=;flags=;id=3c23cf3c-0864-4699-a76b-089350141147;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1577628844607;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada : Links that should pass: )" + getValidLinks().join(' '));
    linkMessages.emplace_back(R"(@badge-info=subscriber/48;badges=broadcaster/1,subscriber/36,partner/1;color=#CC44FF;display-name=pajlada;emotes=;flags=;id=3c23cf3c-0864-4699-a76b-089350141147;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1577628844607;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada : Links that should NOT pass: )" + getInvalidLinks().join(' '));
    linkMessages.emplace_back(R"(@badge-info=subscriber/48;badges=broadcaster/1,subscriber/36,partner/1;color=#CC44FF;display-name=pajlada;emotes=;flags=;id=3c23cf3c-0864-4699-a76b-089350141147;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1577628844607;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #pajlada : Links that should technically pass but we choose not to parse them: )" + getValidButIgnoredLinks().join(' '));

    // channel point reward test
    const char *channelRewardMessage = "{ \"type\": \"MESSAGE\", \"data\": { \"topic\": \"community-points-channel-v1.11148817\", \"message\": { \"type\": \"reward-redeemed\", \"data\": { \"timestamp\": \"2020-07-13T20:19:31.430785354Z\", \"redemption\": { \"id\": \"b9628798-1b4e-4122-b2a6-031658df6755\", \"user\": { \"id\": \"91800084\", \"login\": \"cranken1337\", \"display_name\": \"cranken1337\" }, \"channel_id\": \"11148817\", \"redeemed_at\": \"2020-07-13T20:19:31.345237005Z\", \"reward\": { \"id\": \"313969fe-cc9f-4a0a-83c6-172acbd96957\", \"channel_id\": \"11148817\", \"title\": \"annoying reward pogchamp\", \"prompt\": \"\", \"cost\": 3000, \"is_user_input_required\": true, \"is_sub_only\": false, \"image\": null, \"default_image\": { \"url_1x\": \"https://static-cdn.jtvnw.net/custom-reward-images/default-1.png\", \"url_2x\": \"https://static-cdn.jtvnw.net/custom-reward-images/default-2.png\", \"url_4x\": \"https://static-cdn.jtvnw.net/custom-reward-images/default-4.png\" }, \"background_color\": \"#52ACEC\", \"is_enabled\": true, \"is_paused\": false, \"is_in_stock\": true, \"max_per_stream\": { \"is_enabled\": false, \"max_per_stream\": 0 }, \"should_redemptions_skip_request_queue\": false, \"template_id\": null, \"updated_for_indicator_at\": \"2020-01-20T04:33:33.624956679Z\" }, \"user_input\": \"wow, amazing reward\", \"status\": \"UNFULFILLED\", \"cursor\": \"Yjk2Mjg3OTgtMWI0ZS00MTIyLWIyYTYtMDMxNjU4ZGY2NzU1X18yMDIwLTA3LTEzVDIwOjE5OjMxLjM0NTIzNzAwNVo=\" } } } } }";
    const char *channelRewardMessage2 = "{ \"type\": \"MESSAGE\", \"data\": { \"topic\": \"community-points-channel-v1.11148817\", \"message\": { \"type\": \"reward-redeemed\", \"data\": { \"timestamp\": \"2020-07-13T20:19:31.430785354Z\", \"redemption\": { \"id\": \"b9628798-1b4e-4122-b2a6-031658df6755\", \"user\": { \"id\": \"91800084\", \"login\": \"cranken1337\", \"display_name\": \"cranken1337\" }, \"channel_id\": \"11148817\", \"redeemed_at\": \"2020-07-13T20:19:31.345237005Z\", \"reward\": { \"id\": \"313969fe-cc9f-4a0a-83c6-172acbd96957\", \"channel_id\": \"11148817\", \"title\": \"annoying reward pogchamp\", \"prompt\": \"\", \"cost\": 3000, \"is_user_input_required\": false, \"is_sub_only\": false, \"image\": null, \"default_image\": { \"url_1x\": \"https://static-cdn.jtvnw.net/custom-reward-images/default-1.png\", \"url_2x\": \"https://static-cdn.jtvnw.net/custom-reward-images/default-2.png\", \"url_4x\": \"https://static-cdn.jtvnw.net/custom-reward-images/default-4.png\" }, \"background_color\": \"#52ACEC\", \"is_enabled\": true, \"is_paused\": false, \"is_in_stock\": true, \"max_per_stream\": { \"is_enabled\": false, \"max_per_stream\": 0 }, \"should_redemptions_skip_request_queue\": false, \"template_id\": null, \"updated_for_indicator_at\": \"2020-01-20T04:33:33.624956679Z\" }, \"status\": \"UNFULFILLED\", \"cursor\": \"Yjk2Mjg3OTgtMWI0ZS00MTIyLWIyYTYtMDMxNjU4ZGY2NzU1X18yMDIwLTA3LTEzVDIwOjE5OjMxLjM0NTIzNzAwNVo=\" } } } } }";
    const char *channelRewardIRCMessage(R"(@badge-info=subscriber/43;badges=subscriber/42;color=#1E90FF;custom-reward-id=313969fe-cc9f-4a0a-83c6-172acbd96957;display-name=Cranken1337;emotes=;flags=;id=3cee3f27-a1d0-44d1-a606-722cebdad08b;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1594756484132;turbo=0;user-id=91800084;user-type= :cranken1337!cranken1337@cranken1337.tmi.twitch.tv PRIVMSG #pajlada :wow, amazing reward)");

    emoteTestMessages.emplace_back(R"(@badge-info=subscriber/3;badges=subscriber/3;color=#0000FF;display-name=Linkoping;emotes=25:40-44;flags=17-26:S.6;id=744f9c58-b180-4f46-bd9e-b515b5ef75c1;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1566335866017;turbo=0;user-id=91673457;user-type= :linkoping!linkoping@linkoping.tmi.twitch.tv PRIVMSG #pajlada :Då kan du begära skadestånd och förtal Kappa)");
    emoteTestMessages.emplace_back(R"(@badge-info=subscriber/1;badges=subscriber/0;color=;display-name=jhoelsc;emotes=301683486:46-58,60-72,74-86/301683544:88-100;flags=0-4:S.6;id=1f1afcdd-d94c-4699-b35f-d214deb1e11a;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1588640587462;turbo=0;user-id=505763008;user-type= :jhoelsc!jhoelsc@jhoelsc.tmi.twitch.tv PRIVMSG #pajlada :pensé que no habría directo que bueno que si staryuukiLove staryuukiLove staryuukiLove staryuukiBits)");
    emoteTestMessages.emplace_back(R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=41:6-13,16-23;flags=;id=97c28382-e8d2-45a0-bb5d-2305fc4ef139;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922036771;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm, Kreygasm)");
    emoteTestMessages.emplace_back(R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=25:24-28/41:6-13,15-22;flags=;id=5a36536b-a952-43f7-9c41-88c829371b7a;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922039721;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm,Kreygasm Kappa (no space then space))");
    emoteTestMessages.emplace_back(R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=25:6-10/1902:12-16/88:18-25;flags=;id=aed9e67e-f8cd-493e-aa6b-da054edd7292;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922042881;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kappa.Keepo.PogChamp.extratext (3 emotes with extra text))");
	emoteTestMessages.emplace_back(R"(@badge-info=;badges=moderator/1,partner/1;color=#5B99FF;display-name=StreamElements;emotes=86:30-39/822112:73-79;flags=22-27:S.5;id=03c3eec9-afd1-4858-a2e0-fccbf6ad8d1a;mod=1;room-id=11148817;subscriber=0;tmi-sent-ts=1588638345928;turbo=0;user-id=100135110;user-type=mod :streamelements!streamelements@streamelements.tmi.twitch.tv PRIVMSG #pajlada :╔ACTION A LOJA AINDA NÃO ESTÁ PRONTA BibleThump , AGUARDE... NOVIDADES EM BREVE FortOne╔)");
	emoteTestMessages.emplace_back(R"(@badge-info=subscriber/20;badges=moderator/1,subscriber/12;color=#19E6E6;display-name=randers;emotes=25:39-43;flags=;id=3ea97f01-abb2-4acf-bdb8-f52e79cd0324;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1588837097115;turbo=0;user-id=40286300;user-type=mod :randers!randers@randers.tmi.twitch.tv PRIVMSG #pajlada :Då kan du begära skadestånd och förtal Kappa)");
	emoteTestMessages.emplace_back(R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=41:6-13,15-22;flags=;id=a3196c7e-be4c-4b49-9c5a-8b8302b50c2a;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922213730;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm,Kreygasm (no space))");
    // clang-format on

    createWindowShortcut(this, "F6", [=] {
        const auto &messages = miscMessages;
        static int index = 0;
        auto app = getApp();
        const auto &msg = messages[index++ % messages.size()];
        app->twitch.server->addFakeMessage(msg);
    });

    createWindowShortcut(this, "F7", [=] {
        const auto &messages = cheerMessages;
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch.server->addFakeMessage(msg);
    });

    createWindowShortcut(this, "F8", [=] {
        const auto &messages = linkMessages;
        static int index = 0;
        auto app = getApp();
        const auto &msg = messages[index++ % messages.size()];
        app->twitch.server->addFakeMessage(msg);
    });

    createWindowShortcut(this, "F9", [=] {
        rapidjson::Document doc;
        auto app = getApp();
        static bool alt = true;
        if (alt)
        {
            doc.Parse(channelRewardMessage);
            app->twitch.server->addFakeMessage(channelRewardIRCMessage);
            app->twitch.pubsub->signals_.pointReward.redeemed.invoke(
                doc["data"]["message"]["data"]["redemption"]);
            alt = !alt;
        }
        else
        {
            doc.Parse(channelRewardMessage2);
            app->twitch.pubsub->signals_.pointReward.redeemed.invoke(
                doc["data"]["message"]["data"]["redemption"]);
            alt = !alt;
        }
    });

    createWindowShortcut(this, "F11", [=] {
        const auto &messages = emoteTestMessages;
        static int index = 0;
        const auto &msg = messages[index++ % messages.size()];
        getApp()->twitch.server->addFakeMessage(msg);
    });

#endif
}  // namespace chatterino

void Window::addShortcuts()
{
    /// Initialize program-wide hotkeys
    // Open settings
    createWindowShortcut(this, "CTRL+P", [this] {
        SettingsDialog::showDialog(this);
    });

    // Switch tab
    createWindowShortcut(this, "CTRL+T", [this] {
        this->notebook_->getOrAddSelectedPage()->appendNewSplit(true);
    });

    // CTRL + 1-8 to open corresponding tab.
    for (auto i = 0; i < 8; i++)
    {
        const auto openTab = [this, i] {
            this->notebook_->selectIndex(i);
        };
        createWindowShortcut(this, QString("CTRL+%1").arg(i + 1).toUtf8(),
                             openTab);
    }

    createWindowShortcut(this, "CTRL+9", [this] {
        this->notebook_->selectLastTab();
    });

    createWindowShortcut(this, "CTRL+TAB", [this] {
        this->notebook_->selectNextTab();
    });
    createWindowShortcut(this, "CTRL+SHIFT+TAB", [this] {
        this->notebook_->selectPreviousTab();
    });

    createWindowShortcut(this, "CTRL+N", [this] {
        if (auto page = dynamic_cast<SplitContainer *>(
                this->notebook_->getSelectedPage()))
        {
            if (auto split = page->getSelectedSplit())
            {
                split->popup();
            }
        }
    });

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
    createWindowShortcut(this, "CTRL+SHIFT+T", [this] {
        this->notebook_->addPage(true);
    });

    // Close tab
    createWindowShortcut(this, "CTRL+SHIFT+W", [this] {
        this->notebook_->removeCurrentPage();
    });

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

    createWindowShortcut(this, "CTRL+H", [] {
        getSettings()->hideSimilar.setValue(!getSettings()->hideSimilar);
        getApp()->windows->forceLayoutChannelViews();
    });

    createWindowShortcut(this, "CTRL+K", [this] {
        auto quickSwitcher =
            new QuickSwitcherPopup(&getApp()->windows->getMainWindow());
        quickSwitcher->show();
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
    connect(prefs, &QAction::triggered, this, [this] {
        SettingsDialog::showDialog(this);
    });

    // Window menu.
    QMenu *windowMenu = mainMenu->addMenu(QString("Window"));

    QAction *nextTab = windowMenu->addAction(QString("Select next tab"));
    nextTab->setShortcuts({QKeySequence("Meta+Tab")});
    connect(nextTab, &QAction::triggered, this, [=] {
        this->notebook_->selectNextTab();
    });

    QAction *prevTab = windowMenu->addAction(QString("Select previous tab"));
    prevTab->setShortcuts({QKeySequence("Meta+Shift+Tab")});
    connect(prevTab, &QAction::triggered, this, [=] {
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
