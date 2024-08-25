#include "Application.hpp"

#include "common/Args.hpp"
#include "common/Channel.hpp"
#include "common/QLogging.hpp"
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
#include "providers/seventv/SeventvAPI.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "singletons/ImageUploader.hpp"
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
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Subscription.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvEventAPI.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/PubSubManager.hpp"
#include "providers/twitch/PubSubMessages.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
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
#include <QDesktopServices>

#include <atomic>

namespace {

using namespace chatterino;

const QString BTTV_LIVE_UPDATES_URL = "wss://sockets.betterttv.net/ws";
const QString SEVENTV_EVENTAPI_URL = "wss://events.7tv.io/v3";

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

const QString TWITCH_PUBSUB_URL = "wss://pubsub-edge.twitch.tv";

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
IApplication *INSTANCE = nullptr;

}  // namespace

namespace chatterino {

static std::atomic<bool> isAppInitialized{false};

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
    , emotes(new Emotes)
    , accounts(new AccountController)
    , hotkeys(new HotkeyController)
    , windows(new WindowManager(paths, _settings, *this->themes, *this->fonts))
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
    , logging(new Logging(_settings))
    , linkResolver(new LinkResolver)
    , streamerMode(new StreamerMode)
#ifdef CHATTERINO_HAVE_PLUGINS
    , plugins(new PluginController(paths))
#endif
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
    assert(isAppInitialized == false);
    isAppInitialized = true;

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
    this->initPubSub();

    this->initBttvLiveUpdates();
    this->initSeventvEventAPI();

    this->streamerMode->start();
}

int Application::run(QApplication &qtApp)
{
    assert(isAppInitialized);

    this->twitch->connect();

    if (!this->args_.isFramelessEmbed)
    {
        this->windows->getMainWindow().show();
    }

    getSettings()->betaUpdates.connect(
        [this] {
            this->updates.checkForUpdates();
        },
        false);

    getSettings()->enableBTTVGlobalEmotes.connect(
        [this] {
            this->bttvEmotes->loadEmotes();
        },
        false);
    getSettings()->enableBTTVChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllBTTVChannelEmotes();
        },
        false);
    getSettings()->enableFFZGlobalEmotes.connect(
        [this] {
            this->ffzEmotes->loadEmotes();
        },
        false);
    getSettings()->enableFFZChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllFFZChannelEmotes();
        },
        false);
    getSettings()->enableSevenTVGlobalEmotes.connect(
        [this] {
            this->seventvEmotes->loadGlobalEmotes();
        },
        false);
    getSettings()->enableSevenTVChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllSevenTVChannelEmotes();
        },
        false);

    return qtApp.exec();
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
    assertInGuiThread();

    return this->twitch.get();
}

ITwitchIrcServer *Application::getTwitchAbstract()
{
    assertInGuiThread();

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

void Application::save()
{
    this->commands->save();
    this->hotkeys->save();
    this->windows->save();
}

void Application::initNm(const Paths &paths)
{
    (void)paths;

#ifdef Q_OS_WIN
#    if defined QT_NO_DEBUG || defined CHATTERINO_DEBUG_NM
    registerNmHost(paths);
    this->nmServer.start();
#    endif
#endif
}

void Application::initPubSub()
{
    // We can safely ignore these signal connections since the twitch object will always
    // be destroyed before the Application
    std::ignore = this->twitchPubSub->moderation.chatCleared.connect(
        [this](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text =
                QString("%1 cleared the chat.").arg(action.source.login);

            postToThread([chan, text] {
                chan->addSystemMessage(text);
            });
        });

    std::ignore = this->twitchPubSub->moderation.modeChanged.connect(
        [this](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text =
                QString("%1 turned %2 %3 mode.")
                    .arg(action.source.login)
                    .arg(action.state == ModeChangedAction::State::On ? "on"
                                                                      : "off")
                    .arg(action.getModeName());

            if (action.duration > 0)
            {
                text += QString(" (%1 seconds)").arg(action.duration);
            }

            postToThread([chan, text] {
                chan->addSystemMessage(text);
            });
        });

    std::ignore = this->twitchPubSub->moderation.moderationStateChanged.connect(
        [this](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text;

            text = QString("%1 %2 %3.")
                       .arg(action.source.login,
                            (action.modded ? "modded" : "unmodded"),
                            action.target.login);

            postToThread([chan, text] {
                chan->addSystemMessage(text);
            });
        });

    std::ignore = this->twitchPubSub->moderation.userBanned.connect(
        [&](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                MessageBuilder msg(action);
                msg->flags.set(MessageFlag::PubSub);
                chan->addOrReplaceTimeout(msg.release());
            });
        });

    std::ignore = this->twitchPubSub->moderation.userWarned.connect(
        [&](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            // TODO: Resolve the moderator's user ID into a full user here, so message can look better
            postToThread([chan, action] {
                MessageBuilder msg(action);
                msg->flags.set(MessageFlag::PubSub);
                chan->addMessage(msg.release(), MessageContext::Original);
            });
        });

    std::ignore = this->twitchPubSub->moderation.messageDeleted.connect(
        [&](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty() || getSettings()->hideDeletionActions)
            {
                return;
            }

            auto msg = MessageBuilder::makeDeletionMessageFromPubSub(action);

            postToThread([chan, msg] {
                auto replaced = false;
                LimitedQueueSnapshot<MessagePtr> snapshot =
                    chan->getMessageSnapshot();
                int snapshotLength = snapshot.size();

                // without parens it doesn't build on windows
                int end = (std::max)(0, snapshotLength - 200);

                for (int i = snapshotLength - 1; i >= end; --i)
                {
                    const auto &s = snapshot[i];
                    if (!s->flags.has(MessageFlag::PubSub) &&
                        s->timeoutUser == msg->timeoutUser)
                    {
                        chan->replaceMessage(s, msg);
                        replaced = true;
                        break;
                    }
                }
                if (!replaced)
                {
                    chan->addMessage(msg, MessageContext::Original);
                }
            });
        });

    std::ignore = this->twitchPubSub->moderation.userUnbanned.connect(
        [&](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder(action).release();

            postToThread([chan, msg] {
                chan->addMessage(msg, MessageContext::Original);
            });
        });

    std::ignore =
        this->twitchPubSub->moderation.suspiciousMessageReceived.connect(
            [&](const auto &action) {
                if (action.treatment ==
                    PubSubLowTrustUsersMessage::Treatment::INVALID)
                {
                    qCWarning(chatterinoTwitch)
                        << "Received suspicious message with unknown "
                           "treatment:"
                        << action.treatmentString;
                    return;
                }

                // monitored chats are received over irc; in the future, we will use pubsub instead
                if (action.treatment !=
                    PubSubLowTrustUsersMessage::Treatment::Restricted)
                {
                    return;
                }

                if (getSettings()->streamerModeHideModActions &&
                    this->getStreamerMode()->isEnabled())
                {
                    return;
                }

                auto chan =
                    this->twitch->getChannelOrEmptyByID(action.channelID);

                if (chan->isEmpty())
                {
                    return;
                }

                auto twitchChannel =
                    std::dynamic_pointer_cast<TwitchChannel>(chan);
                if (!twitchChannel)
                {
                    return;
                }

                postToThread([twitchChannel, action] {
                    const auto p = MessageBuilder::makeLowTrustUserMessage(
                        action, twitchChannel->getName(), twitchChannel.get());
                    twitchChannel->addMessage(p.first,
                                              MessageContext::Original);
                    twitchChannel->addMessage(p.second,
                                              MessageContext::Original);
                });
            });

    std::ignore =
        this->twitchPubSub->moderation.suspiciousTreatmentUpdated.connect(
            [&](const auto &action) {
                if (action.treatment ==
                    PubSubLowTrustUsersMessage::Treatment::INVALID)
                {
                    qCWarning(chatterinoTwitch)
                        << "Received suspicious user update with unknown "
                           "treatment:"
                        << action.treatmentString;
                    return;
                }

                if (action.updatedByUserLogin.isEmpty())
                {
                    return;
                }

                if (getSettings()->streamerModeHideModActions &&
                    this->getStreamerMode()->isEnabled())
                {
                    return;
                }

                auto chan =
                    this->twitch->getChannelOrEmptyByID(action.channelID);
                if (chan->isEmpty())
                {
                    return;
                }

                postToThread([chan, action] {
                    auto msg =
                        MessageBuilder::makeLowTrustUpdateMessage(action);
                    chan->addMessage(msg, MessageContext::Original);
                });
            });

    std::ignore = this->twitchPubSub->moderation.autoModMessageCaught.connect(
        [&](const auto &msg, const QString &channelID) {
            auto chan = this->twitch->getChannelOrEmptyByID(channelID);
            if (chan->isEmpty())
            {
                return;
            }

            switch (msg.type)
            {
                case PubSubAutoModQueueMessage::Type::AutoModCaughtMessage: {
                    if (msg.status == "PENDING")
                    {
                        AutomodAction action(msg.data, channelID);
                        action.reason = QString("%1 level %2")
                                            .arg(msg.contentCategory)
                                            .arg(msg.contentLevel);

                        action.msgID = msg.messageID;
                        action.message = msg.messageText;

                        // this message also contains per-word automod data, which could be implemented

                        // extract sender data manually because Twitch loves not being consistent
                        QString senderDisplayName =
                            msg.senderUserDisplayName;  // Might be transformed later
                        bool hasLocalizedName = false;
                        if (!msg.senderUserDisplayName.isEmpty())
                        {
                            // check for non-ascii display names
                            if (QString::compare(msg.senderUserDisplayName,
                                                 msg.senderUserLogin,
                                                 Qt::CaseInsensitive) != 0)
                            {
                                hasLocalizedName = true;
                            }
                        }
                        QColor senderColor = msg.senderUserChatColor;
                        QString senderColor_;
                        if (!senderColor.isValid() &&
                            getSettings()->colorizeNicknames)
                        {
                            // color may be not present if user is a grey-name
                            senderColor = getRandomColor(msg.senderUserID);
                        }

                        // handle username style based on prefered setting
                        switch (getSettings()->usernameDisplayMode.getValue())
                        {
                            case UsernameDisplayMode::Username: {
                                if (hasLocalizedName)
                                {
                                    senderDisplayName = msg.senderUserLogin;
                                }
                                break;
                            }
                            case UsernameDisplayMode::LocalizedName: {
                                break;
                            }
                            case UsernameDisplayMode::
                                UsernameAndLocalizedName: {
                                if (hasLocalizedName)
                                {
                                    senderDisplayName = QString("%1(%2)").arg(
                                        msg.senderUserLogin,
                                        msg.senderUserDisplayName);
                                }
                                break;
                            }
                        }

                        action.target =
                            ActionUser{msg.senderUserID, msg.senderUserLogin,
                                       senderDisplayName, senderColor};
                        postToThread([chan, action] {
                            const auto p = MessageBuilder::makeAutomodMessage(
                                action, chan->getName());
                            chan->addMessage(p.first, MessageContext::Original);
                            chan->addMessage(p.second,
                                             MessageContext::Original);

                            getApp()
                                ->getTwitch()
                                ->getAutomodChannel()
                                ->addMessage(p.first, MessageContext::Original);
                            getApp()
                                ->getTwitch()
                                ->getAutomodChannel()
                                ->addMessage(p.second,
                                             MessageContext::Original);

                            if (getSettings()->showAutomodInMentions)
                            {
                                getApp()
                                    ->getTwitch()
                                    ->getMentionsChannel()
                                    ->addMessage(p.first,
                                                 MessageContext::Original);
                                getApp()
                                    ->getTwitch()
                                    ->getMentionsChannel()
                                    ->addMessage(p.second,
                                                 MessageContext::Original);
                            }
                        });
                    }
                    // "ALLOWED" and "DENIED" statuses remain unimplemented
                    // They are versions of automod_message_(denied|approved) but for mods.
                }
                break;

                case PubSubAutoModQueueMessage::Type::INVALID:
                default: {
                }
                break;
            }
        });

    std::ignore = this->twitchPubSub->moderation.autoModMessageBlocked.connect(
        [&](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                const auto p =
                    MessageBuilder::makeAutomodMessage(action, chan->getName());
                chan->addMessage(p.first, MessageContext::Original);
                chan->addMessage(p.second, MessageContext::Original);
            });
        });

    std::ignore = this->twitchPubSub->moderation.automodUserMessage.connect(
        [&](const auto &action) {
            if (getSettings()->streamerModeHideModActions &&
                this->getStreamerMode()->isEnabled())
            {
                return;
            }
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder(action).release();

            postToThread([chan, msg] {
                chan->addMessage(msg, MessageContext::Original);
            });
            chan->deleteMessage(msg->id);
        });

    std::ignore = this->twitchPubSub->moderation.automodInfoMessage.connect(
        [&](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                const auto p = MessageBuilder::makeAutomodInfoMessage(action);
                chan->addMessage(p, MessageContext::Original);
            });
        });

    std::ignore =
        this->twitchPubSub->pointReward.redeemed.connect([&](auto &data) {
            QString channelId = data.value("channel_id").toString();
            if (channelId.isEmpty())
            {
                qCDebug(chatterinoApp)
                    << "Couldn't find channel id of point reward";
                return;
            }

            auto chan = this->twitch->getChannelOrEmptyByID(channelId);

            auto reward = ChannelPointReward(data);

            postToThread([chan, reward] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->addChannelPointReward(reward);
                }
            });
        });

    this->twitchPubSub->start();
    this->twitchPubSub->setAccount(this->accounts->twitch.getCurrent());

    this->accounts->twitch.currentUserChanged.connect(
        [this] {
            this->twitchPubSub->unlistenChannelModerationActions();
            this->twitchPubSub->unlistenAutomod();
            this->twitchPubSub->unlistenLowTrustUsers();
            this->twitchPubSub->unlistenChannelPointRewards();

            this->twitchPubSub->setAccount(this->accounts->twitch.getCurrent());
        },
        boost::signals2::at_front);
}

void Application::initBttvLiveUpdates()
{
    if (!this->bttvLiveUpdates)
    {
        qCDebug(chatterinoBttv)
            << "Skipping initialization of Live Updates as it's disabled";
        return;
    }

    // We can safely ignore these signal connections since the twitch object will always
    // be destroyed before the Application
    std::ignore = this->bttvLiveUpdates->signals_.emoteAdded.connect(
        [&](const auto &data) {
            auto chan = this->twitch->getChannelOrEmptyByID(data.channelID);

            postToThread([chan, data] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->addBttvEmote(data);
                }
            });
        });
    std::ignore = this->bttvLiveUpdates->signals_.emoteUpdated.connect(
        [&](const auto &data) {
            auto chan = this->twitch->getChannelOrEmptyByID(data.channelID);

            postToThread([chan, data] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->updateBttvEmote(data);
                }
            });
        });
    std::ignore = this->bttvLiveUpdates->signals_.emoteRemoved.connect(
        [&](const auto &data) {
            auto chan = this->twitch->getChannelOrEmptyByID(data.channelID);

            postToThread([chan, data] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->removeBttvEmote(data);
                }
            });
        });
    this->bttvLiveUpdates->start();
}

void Application::initSeventvEventAPI()
{
    if (!this->seventvEventAPI)
    {
        qCDebug(chatterinoSeventvEventAPI)
            << "Skipping initialization as the EventAPI is disabled";
        return;
    }

    // We can safely ignore these signal connections since the twitch object will always
    // be destroyed before the Application
    std::ignore = this->seventvEventAPI->signals_.emoteAdded.connect(
        [&](const auto &data) {
            postToThread([this, data] {
                this->twitch->forEachSeventvEmoteSet(
                    data.emoteSetID, [data](TwitchChannel &chan) {
                        chan.addSeventvEmote(data);
                    });
            });
        });
    std::ignore = this->seventvEventAPI->signals_.emoteUpdated.connect(
        [&](const auto &data) {
            postToThread([this, data] {
                this->twitch->forEachSeventvEmoteSet(
                    data.emoteSetID, [data](TwitchChannel &chan) {
                        chan.updateSeventvEmote(data);
                    });
            });
        });
    std::ignore = this->seventvEventAPI->signals_.emoteRemoved.connect(
        [&](const auto &data) {
            postToThread([this, data] {
                this->twitch->forEachSeventvEmoteSet(
                    data.emoteSetID, [data](TwitchChannel &chan) {
                        chan.removeSeventvEmote(data);
                    });
            });
        });
    std::ignore = this->seventvEventAPI->signals_.userUpdated.connect(
        [&](const auto &data) {
            this->twitch->forEachSeventvUser(data.userID,
                                             [data](TwitchChannel &chan) {
                                                 chan.updateSeventvUser(data);
                                             });
        });

    this->seventvEventAPI->start();
}

IApplication *getApp()
{
    assert(INSTANCE != nullptr);

    return INSTANCE;
}

}  // namespace chatterino
