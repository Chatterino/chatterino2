#include "Application.hpp"

#include "common/Args.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginController.hpp"
#endif
#include "controllers/sound/SoundController.hpp"
#include "controllers/twitch/LiveController.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/bttv/BttvLiveUpdates.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/irc/Irc2.hpp"
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
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/helper/LoggingChannel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
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

namespace chatterino {

static std::atomic<bool> isAppInitialized{false};

Application *Application::instance = nullptr;
IApplication *IApplication::instance = nullptr;

IApplication::IApplication()
{
    IApplication::instance = this;
}

// this class is responsible for handling the workflow of Chatterino
// It will create the instances of the major classes, and connect their signals
// to each other

Application::Application(Settings &_settings, Paths &_paths)
    : themes(&this->emplace<Theme>())
    , fonts(&this->emplace<Fonts>())
    , emotes(&this->emplace<Emotes>())
    , accounts(&this->emplace<AccountController>())
    , hotkeys(&this->emplace<HotkeyController>())
    , windows(&this->emplace<WindowManager>())
    , toasts(&this->emplace<Toasts>())

    , commands(&this->emplace<CommandController>())
    , notifications(&this->emplace<NotificationController>())
    , highlights(&this->emplace<HighlightController>())
    , twitch(&this->emplace<TwitchIrcServer>())
    , chatterinoBadges(&this->emplace<ChatterinoBadges>())
    , ffzBadges(&this->emplace<FfzBadges>())
    , seventvBadges(&this->emplace<SeventvBadges>())
    , userData(&this->emplace<UserDataController>())
    , sound(&this->emplace<SoundController>())
    , twitchLiveController(&this->emplace<TwitchLiveController>())
#ifdef CHATTERINO_HAVE_PLUGINS
    , plugins(&this->emplace<PluginController>())
#endif
    , logging(&this->emplace<Logging>())
{
    this->instance = this;

    // We can safely ignore this signal's connection since the Application will always
    // be destroyed after fonts
    std::ignore = this->fonts->fontChanged.connect([this]() {
        this->windows->layoutChannelViews();
    });
}

void Application::initialize(Settings &settings, Paths &paths)
{
    assert(isAppInitialized == false);
    isAppInitialized = true;

    // Show changelog
    if (!getArgs().isFramelessEmbed &&
        getSettings()->currentVersion.getValue() != "" &&
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

    if (!getArgs().isFramelessEmbed)
    {
        getSettings()->currentVersion.setValue(CHATTERINO_VERSION);

        if (getSettings()->enableExperimentalIrc)
        {
            Irc::instance().load();
        }
    }

    for (auto &singleton : this->singletons_)
    {
        singleton->initialize(settings, paths);
    }

    // add crash message
    if (!getArgs().isFramelessEmbed && getArgs().crashRecovery)
    {
        if (auto selected =
                this->windows->getMainWindow().getNotebook().getSelectedPage())
        {
            if (auto container = dynamic_cast<SplitContainer *>(selected))
            {
                for (auto &&split : container->getSplits())
                {
                    if (auto channel = split->getChannel(); !channel->isEmpty())
                    {
                        channel->addMessage(makeSystemMessage(
                            "Chatterino unexpectedly crashed and restarted. "
                            "You can disable automatic restarts in the "
                            "settings."));
                    }
                }
            }
        }
    }

    this->windows->updateWordTypeMask();

    if (!getArgs().isFramelessEmbed)
    {
        this->initNm(paths);
    }
    this->initPubSub();

    this->initBttvLiveUpdates();
    this->initSeventvEventAPI();
}

int Application::run(QApplication &qtApp)
{
    assert(isAppInitialized);

    this->twitch->connect();

    if (!getArgs().isFramelessEmbed)
    {
        this->windows->getMainWindow().show();
    }

    getSettings()->betaUpdates.connect(
        [] {
            Updates::instance().checkForUpdates();
        },
        false);

    // We can safely ignore the signal connections since Application will always live longer than
    // everything else, including settings. right?
    // NOTE: SETTINGS_LIFETIME
    std::ignore =
        getSettings()->moderationActions.delayedItemsChanged.connect([this] {
            this->windows->forceLayoutChannelViews();
        });

    std::ignore =
        getSettings()->highlightedMessages.delayedItemsChanged.connect([this] {
            this->windows->forceLayoutChannelViews();
        });
    std::ignore =
        getSettings()->highlightedUsers.delayedItemsChanged.connect([this] {
            this->windows->forceLayoutChannelViews();
        });

    getSettings()->removeSpacesBetweenEmotes.connect([this] {
        this->windows->forceLayoutChannelViews();
    });

    getSettings()->enableBTTVGlobalEmotes.connect(
        [this] {
            this->twitch->reloadBTTVGlobalEmotes();
        },
        false);
    getSettings()->enableBTTVChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllBTTVChannelEmotes();
        },
        false);
    getSettings()->enableFFZGlobalEmotes.connect(
        [this] {
            this->twitch->reloadFFZGlobalEmotes();
        },
        false);
    getSettings()->enableFFZChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllFFZChannelEmotes();
        },
        false);
    getSettings()->enableSevenTVGlobalEmotes.connect(
        [this] {
            this->twitch->reloadSevenTVGlobalEmotes();
        },
        false);
    getSettings()->enableSevenTVChannelEmotes.connect(
        [this] {
            this->twitch->reloadAllSevenTVChannelEmotes();
        },
        false);

    return qtApp.exec();
}

IEmotes *Application::getEmotes()
{
    return this->emotes;
}

IUserDataController *Application::getUserData()
{
    return this->userData;
}

ITwitchLiveController *Application::getTwitchLiveController()
{
    return this->twitchLiveController;
}

ITwitchIrcServer *Application::getTwitch()
{
    return this->twitch;
}

void Application::save()
{
    for (auto &singleton : this->singletons_)
    {
        singleton->save();
    }
}

void Application::initNm(Paths &paths)
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
    std::ignore = this->twitch->pubsub->signals_.moderation.chatCleared.connect(
        [this](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text =
                QString("%1 cleared the chat").arg(action.source.login);

            auto msg = makeSystemMessage(text);
            postToThread([chan, msg] {
                chan->addMessage(msg);
            });
        });

    std::ignore = this->twitch->pubsub->signals_.moderation.modeChanged.connect(
        [this](const auto &action) {
            auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
            if (chan->isEmpty())
            {
                return;
            }

            QString text =
                QString("%1 turned %2 %3 mode")
                    .arg(action.source.login)
                    .arg(action.state == ModeChangedAction::State::On ? "on"
                                                                      : "off")
                    .arg(action.getModeName());

            if (action.duration > 0)
            {
                text += QString(" (%1 seconds)").arg(action.duration);
            }

            auto msg = makeSystemMessage(text);
            postToThread([chan, msg] {
                chan->addMessage(msg);
            });
        });

    std::ignore =
        this->twitch->pubsub->signals_.moderation.moderationStateChanged
            .connect([this](const auto &action) {
                auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
                if (chan->isEmpty())
                {
                    return;
                }

                QString text;

                text = QString("%1 %2 %3")
                           .arg(action.source.login,
                                (action.modded ? "modded" : "unmodded"),
                                action.target.login);

                auto msg = makeSystemMessage(text);
                postToThread([chan, msg] {
                    chan->addMessage(msg);
                });
            });

    std::ignore = this->twitch->pubsub->signals_.moderation.userBanned.connect(
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
    std::ignore =
        this->twitch->pubsub->signals_.moderation.messageDeleted.connect(
            [&](const auto &action) {
                auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

                if (chan->isEmpty() || getSettings()->hideDeletionActions)
                {
                    return;
                }

                MessageBuilder msg;
                TwitchMessageBuilder::deletionMessage(action, &msg);
                msg->flags.set(MessageFlag::PubSub);

                postToThread([chan, msg = msg.release()] {
                    auto replaced = false;
                    LimitedQueueSnapshot<MessagePtr> snapshot =
                        chan->getMessageSnapshot();
                    int snapshotLength = snapshot.size();

                    // without parens it doesn't build on windows
                    int end = (std::max)(0, snapshotLength - 200);

                    for (int i = snapshotLength - 1; i >= end; --i)
                    {
                        auto &s = snapshot[i];
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
                        chan->addMessage(msg);
                    }
                });
            });

    std::ignore =
        this->twitch->pubsub->signals_.moderation.userUnbanned.connect(
            [&](const auto &action) {
                auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

                if (chan->isEmpty())
                {
                    return;
                }

                auto msg = MessageBuilder(action).release();

                postToThread([chan, msg] {
                    chan->addMessage(msg);
                });
            });

    std::ignore =
        this->twitch->pubsub->signals_.moderation.autoModMessageCaught.connect(
            [&](const auto &msg, const QString &channelID) {
                auto chan = this->twitch->getChannelOrEmptyByID(channelID);
                if (chan->isEmpty())
                {
                    return;
                }

                switch (msg.type)
                {
                    case PubSubAutoModQueueMessage::Type::
                        AutoModCaughtMessage: {
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
                            switch (
                                getSettings()->usernameDisplayMode.getValue())
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
                                        senderDisplayName =
                                            QString("%1(%2)").arg(
                                                msg.senderUserLogin,
                                                msg.senderUserDisplayName);
                                    }
                                    break;
                                }
                            }

                            action.target = ActionUser{
                                msg.senderUserID, msg.senderUserLogin,
                                senderDisplayName, senderColor};
                            postToThread([chan, action] {
                                const auto p = makeAutomodMessage(action);
                                chan->addMessage(p.first);
                                chan->addMessage(p.second);
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

    std::ignore =
        this->twitch->pubsub->signals_.moderation.autoModMessageBlocked.connect(
            [&](const auto &action) {
                auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);
                if (chan->isEmpty())
                {
                    return;
                }

                postToThread([chan, action] {
                    const auto p = makeAutomodMessage(action);
                    chan->addMessage(p.first);
                    chan->addMessage(p.second);
                });
            });

    std::ignore =
        this->twitch->pubsub->signals_.moderation.automodUserMessage.connect(
            [&](const auto &action) {
                // This condition has been set up to execute isInStreamerMode() as the last thing
                // as it could end up being expensive.
                if (getSettings()->streamerModeHideModActions &&
                    isInStreamerMode())
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
                    chan->addMessage(msg);
                });
                chan->deleteMessage(msg->id);
            });

    std::ignore =
        this->twitch->pubsub->signals_.moderation.automodInfoMessage.connect(
            [&](const auto &action) {
                auto chan = this->twitch->getChannelOrEmptyByID(action.roomID);

                if (chan->isEmpty())
                {
                    return;
                }

                postToThread([chan, action] {
                    const auto p = makeAutomodInfoMessage(action);
                    chan->addMessage(p);
                });
            });

    std::ignore = this->twitch->pubsub->signals_.pointReward.redeemed.connect(
        [&](auto &data) {
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
                if (auto channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->addChannelPointReward(reward);
                }
            });
        });

    this->twitch->pubsub->start();

    auto RequestModerationActions = [this]() {
        this->twitch->pubsub->setAccount(
            getApp()->accounts->twitch.getCurrent());
        // TODO(pajlada): Unlisten to all authed topics instead of only
        // moderation topics this->twitch->pubsub->UnlistenAllAuthedTopics();

        this->twitch->pubsub->listenToWhispers();
    };

    this->accounts->twitch.currentUserChanged.connect(
        [this] {
            this->twitch->pubsub->unlistenAllModerationActions();
            this->twitch->pubsub->unlistenAutomod();
            this->twitch->pubsub->unlistenWhispers();
        },
        boost::signals2::at_front);

    this->accounts->twitch.currentUserChanged.connect(RequestModerationActions);

    RequestModerationActions();
}

void Application::initBttvLiveUpdates()
{
    if (!this->twitch->bttvLiveUpdates)
    {
        qCDebug(chatterinoBttv)
            << "Skipping initialization of Live Updates as it's disabled";
        return;
    }

    // We can safely ignore these signal connections since the twitch object will always
    // be destroyed before the Application
    std::ignore = this->twitch->bttvLiveUpdates->signals_.emoteAdded.connect(
        [&](const auto &data) {
            auto chan = this->twitch->getChannelOrEmptyByID(data.channelID);

            postToThread([chan, data] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->addBttvEmote(data);
                }
            });
        });
    std::ignore = this->twitch->bttvLiveUpdates->signals_.emoteUpdated.connect(
        [&](const auto &data) {
            auto chan = this->twitch->getChannelOrEmptyByID(data.channelID);

            postToThread([chan, data] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->updateBttvEmote(data);
                }
            });
        });
    std::ignore = this->twitch->bttvLiveUpdates->signals_.emoteRemoved.connect(
        [&](const auto &data) {
            auto chan = this->twitch->getChannelOrEmptyByID(data.channelID);

            postToThread([chan, data] {
                if (auto *channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->removeBttvEmote(data);
                }
            });
        });
    this->twitch->bttvLiveUpdates->start();
}

void Application::initSeventvEventAPI()
{
    if (!this->twitch->seventvEventAPI)
    {
        qCDebug(chatterinoSeventvEventAPI)
            << "Skipping initialization as the EventAPI is disabled";
        return;
    }

    // We can safely ignore these signal connections since the twitch object will always
    // be destroyed before the Application
    std::ignore = this->twitch->seventvEventAPI->signals_.emoteAdded.connect(
        [&](const auto &data) {
            postToThread([this, data] {
                this->twitch->forEachSeventvEmoteSet(
                    data.emoteSetID, [data](TwitchChannel &chan) {
                        chan.addSeventvEmote(data);
                    });
            });
        });
    std::ignore = this->twitch->seventvEventAPI->signals_.emoteUpdated.connect(
        [&](const auto &data) {
            postToThread([this, data] {
                this->twitch->forEachSeventvEmoteSet(
                    data.emoteSetID, [data](TwitchChannel &chan) {
                        chan.updateSeventvEmote(data);
                    });
            });
        });
    std::ignore = this->twitch->seventvEventAPI->signals_.emoteRemoved.connect(
        [&](const auto &data) {
            postToThread([this, data] {
                this->twitch->forEachSeventvEmoteSet(
                    data.emoteSetID, [data](TwitchChannel &chan) {
                        chan.removeSeventvEmote(data);
                    });
            });
        });
    std::ignore = this->twitch->seventvEventAPI->signals_.userUpdated.connect(
        [&](const auto &data) {
            this->twitch->forEachSeventvUser(data.userID,
                                             [data](TwitchChannel &chan) {
                                                 chan.updateSeventvUser(data);
                                             });
        });

    this->twitch->seventvEventAPI->start();
}

Application *getApp()
{
    assert(Application::instance != nullptr);

    assertInGuiThread();

    return Application::instance;
}

IApplication *getIApp()
{
    assert(IApplication::instance != nullptr);

    assertInGuiThread();

    return IApplication::instance;
}

}  // namespace chatterino
