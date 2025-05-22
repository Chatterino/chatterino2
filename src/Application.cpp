#include "Application.hpp"

#include "common/Args.hpp"
#include "common/Channel.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "controllers/sound/ISoundController.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/links/LinkResolver.hpp"
#include "providers/pronouns/Pronouns.hpp"
#include "providers/seventv/SeventvAPI.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/eventsub/Controller.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "singletons/ImageUploader.hpp"
#include "singletons/NativeMessaging.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginController.hpp"
#endif
#include "controllers/sound/MiniaudioBackend.hpp"
#include "controllers/sound/NullBackend.hpp"
#include "controllers/twitch/LiveController.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/bttv/BttvLiveUpdates.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvEventAPI.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/PubSubMessages.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchUsers.hpp"
#include "singletons/CrashHandler.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/helper/LoggingChannel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/Updates.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/PostToThread.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/Window.hpp"

#include <miniaudio.h>
#include <QApplication>
#include <QDesktopServices>

namespace {

using namespace chatterino;

const QString BTTV_LIVE_UPDATES_URL = "wss://sockets.betterttv.net/ws";
const QString SEVENTV_EVENTAPI_URL = "wss://events.7tv.io/v3";

std::atomic<bool> STOPPED{false};
std::atomic<bool> ABOUT_TO_QUIT{false};

ISoundController *makeSoundController(Settings &settings)
{
    SoundBackend soundBackend = settings.soundBackend;
    switch (soundBackend)
    {
        case SoundBackend::Miniaudio: {
            return new MiniaudioBackend();
        }
        break;

        case SoundBackend::Null: {
            return new NullBackend();
        }
        break;

        default: {
            return new MiniaudioBackend();
        }
        break;
    }
}

BttvLiveUpdates *makeBttvLiveUpdates(Settings &settings)
{
    bool enabled =
        settings.enableBTTVLiveUpdates && settings.enableBTTVChannelEmotes;

    if (enabled)
    {
        return new BttvLiveUpdates(BTTV_LIVE_UPDATES_URL);
    }

    return nullptr;
}

SeventvEventAPI *makeSeventvEventAPI(Settings &settings)
{
    bool enabled = settings.enableSevenTVEventAPI;

    if (enabled)
    {
        return new SeventvEventAPI(SEVENTV_EVENTAPI_URL);
    }

    return nullptr;
}

eventsub::IController *makeEventSubController(Settings &settings)
{
    bool enabled = settings.enableExperimentalEventSub;

    if (enabled)
    {
        return new eventsub::Controller();
    }

    return new eventsub::DummyController();
}

const QString TWITCH_PUBSUB_URL = "wss://pubsub-edge.twitch.tv";

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
IApplication *INSTANCE = nullptr;

}  // namespace

namespace chatterino {

IApplication::IApplication()
{
    INSTANCE = this;
}

IApplication::~IApplication()
{
    INSTANCE = nullptr;
}

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals
// to each other

Application::Application(Settings &_settings, const Paths &paths,
                         const Args &_args, Updates &_updates)
    : paths_(paths)
    , args_(_args)
    , themes(new Theme(paths))
    , fonts(new Fonts(_settings))
    , logging(new Logging(_settings))
    , emotes(new Emotes)
    , accounts(new AccountController)
    , eventSub(makeEventSubController(_settings))
    , hotkeys(new HotkeyController)
    , windows(new WindowManager(_args, paths, _settings, *this->themes,
                                *this->fonts))
    , toasts(new Toasts)
    , imageUploader(new ImageUploader)
    , seventvAPI(new SeventvAPI)
    , crashHandler(new CrashHandler(paths))

    , commands(new CommandController(paths))
    , notifications(new NotificationController)
    , highlights(new HighlightController(_settings, this->accounts.get()))
    , twitch(new TwitchIrcServer)
    , ffzBadges(new FfzBadges)
    , seventvBadges(new SeventvBadges)
    , userData(new UserDataController(paths))
    , sound(makeSoundController(_settings))
    , twitchLiveController(new TwitchLiveController)
    , twitchPubSub(new PubSub(TWITCH_PUBSUB_URL))
    , twitchBadges(new TwitchBadges)
    , chatterinoBadges(new ChatterinoBadges)
    , bttvEmotes(new BttvEmotes)
    , bttvLiveUpdates(makeBttvLiveUpdates(_settings))
    , ffzEmotes(new FfzEmotes)
    , seventvEmotes(new SeventvEmotes)
    , seventvEventAPI(makeSeventvEventAPI(_settings))
    , linkResolver(new LinkResolver)
    , streamerMode(new StreamerMode)
    , twitchUsers(new TwitchUsers)
    , pronouns(new pronouns::Pronouns)
#ifdef CHATTERINO_HAVE_PLUGINS
    , plugins(new PluginController(paths))
#endif
    , nmServer(new NativeMessagingServer())
    , updates(_updates)
{
}

Application::~Application()
{
    // we do this early to ensure getApp isn't used in any dtors
    INSTANCE = nullptr;
}

void Application::initialize(Settings &settings, const Paths &paths)
{
    assert(!this->initialized);

    // Show changelog
    if (!this->args_.isFramelessEmbed &&
        getSettings()->currentVersion.getValue() != "" &&
        getSettings()->currentVersion.getValue() != CHATTERINO_VERSION)
    {
        auto *box = new QMessageBox(QMessageBox::Information, "Chatterino 2",
                                    "Show changelog?",
                                    QMessageBox::Yes | QMessageBox::No);
        box->setAttribute(Qt::WA_DeleteOnClose);
        if (box->exec() == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(
                QUrl("https://www.chatterino.com/changelog"));
        }
    }

    if (!this->args_.isFramelessEmbed)
    {
        getSettings()->currentVersion.setValue(CHATTERINO_VERSION);
    }

    this->accounts->load();

    this->windows->initialize();

    this->ffzBadges->load();

    // Load global emotes
    this->bttvEmotes->loadEmotes();
    this->ffzEmotes->loadEmotes();
    this->seventvEmotes->loadGlobalEmotes();

    this->twitch->initialize();

    // Load live status
    this->notifications->initialize();

    // XXX: Loading Twitch badges after Helix has been initialized, which only happens after
    // the AccountController initialize has been called
    this->twitchBadges->loadTwitchBadges();

#ifdef CHATTERINO_HAVE_PLUGINS
    this->plugins->initialize(settings);
#endif

    // Show crash message.
    // On Windows, the crash message was already shown.
#ifndef Q_OS_WIN
    if (!this->args_.isFramelessEmbed && this->args_.crashRecovery)
    {
        if (auto *selected =
                this->windows->getMainWindow().getNotebook().getSelectedPage())
        {
            if (auto *container = dynamic_cast<SplitContainer *>(selected))
            {
                for (auto &&split : container->getSplits())
                {
                    if (auto channel = split->getChannel(); !channel->isEmpty())
                    {
                        channel->addSystemMessage(
                            "Chatterino unexpectedly crashed and restarted. "
                            "You can disable automatic restarts in the "
                            "settings.");
                    }
                }
            }
        }
    }
#endif

    if (!this->args_.isFramelessEmbed)
    {
        this->initNm(paths);
    }
    this->twitchPubSub->initialize();

    this->twitch->initEventAPIs(this->bttvLiveUpdates.get(),
                                this->seventvEventAPI.get());

    this->streamerMode->start();

    this->initialized = true;
}

int Application::run()
{
    assert(this->initialized);

    this->twitch->connect();

    if (!this->args_.isFramelessEmbed)
    {
        this->windows->getMainWindow().show();
    }

    getSettings()->enableBTTVChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllBTTVChannelEmotes();
        },
        false);
    getSettings()->enableFFZChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllFFZChannelEmotes();
        },
        false);
    getSettings()->enableSevenTVChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllSevenTVChannelEmotes();
        },
        false);

    return QApplication::exec();
}

Theme *Application::getThemes()
{
    assertInGuiThread();
    assert(this->themes);

    return this->themes.get();
}

Fonts *Application::getFonts()
{
    assertInGuiThread();
    assert(this->fonts);

    return this->fonts.get();
}

IEmotes *Application::getEmotes()
{
    assertInGuiThread();
    assert(this->emotes);

    return this->emotes.get();
}

AccountController *Application::getAccounts()
{
    assertInGuiThread();
    assert(this->accounts);

    return this->accounts.get();
}

HotkeyController *Application::getHotkeys()
{
    assertInGuiThread();
    assert(this->hotkeys);

    return this->hotkeys.get();
}

WindowManager *Application::getWindows()
{
    assertInGuiThread();
    assert(this->windows);

    return this->windows.get();
}

Toasts *Application::getToasts()
{
    assertInGuiThread();
    assert(this->toasts);

    return this->toasts.get();
}

CrashHandler *Application::getCrashHandler()
{
    assertInGuiThread();
    assert(this->crashHandler);

    return this->crashHandler.get();
}

CommandController *Application::getCommands()
{
    assertInGuiThread();
    assert(this->commands);

    return this->commands.get();
}

NotificationController *Application::getNotifications()
{
    assertInGuiThread();
    assert(this->notifications);

    return this->notifications.get();
}

HighlightController *Application::getHighlights()
{
    assertInGuiThread();
    assert(this->highlights);

    return this->highlights.get();
}

FfzBadges *Application::getFfzBadges()
{
    assertInGuiThread();
    assert(this->ffzBadges);

    return this->ffzBadges.get();
}

SeventvBadges *Application::getSeventvBadges()
{
    // SeventvBadges handles its own locks, so we don't need to assert that this is called in the GUI thread
    assert(this->seventvBadges);

    return this->seventvBadges.get();
}

IUserDataController *Application::getUserData()
{
    assertInGuiThread();

    return this->userData.get();
}

ISoundController *Application::getSound()
{
    assertInGuiThread();

    return this->sound.get();
}

ITwitchLiveController *Application::getTwitchLiveController()
{
    assertInGuiThread();
    assert(this->twitchLiveController);

    return this->twitchLiveController.get();
}

TwitchBadges *Application::getTwitchBadges()
{
    assertInGuiThread();
    assert(this->twitchBadges);

    return this->twitchBadges.get();
}

IChatterinoBadges *Application::getChatterinoBadges()
{
    assertInGuiThread();
    assert(this->chatterinoBadges);

    return this->chatterinoBadges.get();
}

ImageUploader *Application::getImageUploader()
{
    assertInGuiThread();
    assert(this->imageUploader);

    return this->imageUploader.get();
}

SeventvAPI *Application::getSeventvAPI()
{
    assertInGuiThread();
    assert(this->seventvAPI);

    return this->seventvAPI.get();
}

#ifdef CHATTERINO_HAVE_PLUGINS
PluginController *Application::getPlugins()
{
    assertInGuiThread();
    assert(this->plugins);

    return this->plugins.get();
}
#endif

Updates &Application::getUpdates()
{
    assertInGuiThread();

    return this->updates;
}

ITwitchIrcServer *Application::getTwitch()
{
    return this->twitch.get();
}

PubSub *Application::getTwitchPubSub()
{
    assertInGuiThread();

    return this->twitchPubSub.get();
}

ILogging *Application::getChatLogger()
{
    assertInGuiThread();
    assert(this->logging);

    return this->logging.get();
}

ILinkResolver *Application::getLinkResolver()
{
    assertInGuiThread();

    return this->linkResolver.get();
}

IStreamerMode *Application::getStreamerMode()
{
    return this->streamerMode.get();
}

ITwitchUsers *Application::getTwitchUsers()
{
    assertInGuiThread();
    assert(this->twitchUsers);

    return this->twitchUsers.get();
}

BttvEmotes *Application::getBttvEmotes()
{
    assertInGuiThread();
    assert(this->bttvEmotes);

    return this->bttvEmotes.get();
}

BttvLiveUpdates *Application::getBttvLiveUpdates()
{
    assertInGuiThread();
    // bttvLiveUpdates may be nullptr if it's not enabled

    return this->bttvLiveUpdates.get();
}

FfzEmotes *Application::getFfzEmotes()
{
    assertInGuiThread();
    assert(this->ffzEmotes);

    return this->ffzEmotes.get();
}

SeventvEmotes *Application::getSeventvEmotes()
{
    assertInGuiThread();
    assert(this->seventvEmotes);

    return this->seventvEmotes.get();
}

SeventvEventAPI *Application::getSeventvEventAPI()
{
    assertInGuiThread();
    // seventvEventAPI may be nullptr if it's not enabled

    return this->seventvEventAPI.get();
}

pronouns::Pronouns *Application::getPronouns()
{
    // pronouns::Pronouns handles its own locks, so we don't need to assert that this is called in the GUI thread
    assert(this->pronouns);

    return this->pronouns.get();
}

eventsub::IController *Application::getEventSub()
{
    assert(this->eventSub);

    return this->eventSub.get();
}

void Application::aboutToQuit()
{
    ABOUT_TO_QUIT.store(true);

    this->eventSub->setQuitting();

    this->twitch->aboutToQuit();

    this->hotkeys->save();
    this->windows->save();
}

void Application::stop()
{
#ifdef CHATTERINO_HAVE_PLUGINS
    this->plugins.reset();
#endif
    this->pronouns.reset();
    this->twitchUsers.reset();
    this->streamerMode.reset();
    this->linkResolver.reset();
    this->seventvEventAPI.reset();
    this->seventvEmotes.reset();
    this->ffzEmotes.reset();
    this->bttvLiveUpdates.reset();
    this->bttvEmotes.reset();
    this->chatterinoBadges.reset();
    this->twitchBadges.reset();
    this->twitchPubSub.reset();
    this->twitchLiveController.reset();
    this->sound.reset();
    this->userData.reset();
    this->seventvBadges.reset();
    this->ffzBadges.reset();
    this->twitch.reset();
    this->highlights.reset();
    this->notifications.reset();
    this->commands.reset();
    this->crashHandler.reset();
    this->seventvAPI.reset();
    this->imageUploader.reset();
    this->toasts.reset();
    this->windows.reset();
    this->hotkeys.reset();
    this->eventSub.reset();
    this->accounts.reset();
    this->emotes.reset();
    this->logging.reset();
    this->fonts.reset();
    this->themes.reset();

    STOPPED.store(true);
}

void Application::initNm(const Paths &paths)
{
    (void)paths;

#if defined QT_NO_DEBUG || defined CHATTERINO_DEBUG_NM
    registerNmHost(paths);
    this->nmServer->start();
#endif
}

IApplication *getApp()
{
    assert(INSTANCE != nullptr);
    assert(STOPPED.load() == false);

    return INSTANCE;
}

IApplication *tryGetApp()
{
    return INSTANCE;
}

bool isAppAboutToQuit()
{
    return ABOUT_TO_QUIT.load();
}

}  // namespace chatterino
