#include "Application.hpp"

#include <atomic>

#include "common/Args.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/irc/Irc2.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Logging.hpp"
#include "singletons/NativeMessaging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/Updates.hpp"
#include "singletons/WindowManager.hpp"
#include "util/IsBigEndian.hpp"
#include "util/PostToThread.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/splits/Split.hpp"

#include <QDesktopServices>

namespace chatterino {

static std::atomic<bool> isAppInitialized{false};

Application *Application::instance = nullptr;

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
    , twitch2(&this->emplace<TwitchIrcServer>())
    , chatterinoBadges(&this->emplace<ChatterinoBadges>())
    , ffzBadges(&this->emplace<FfzBadges>())
    , logging(&this->emplace<Logging>())
{
    this->instance = this;

    this->fonts->fontChanged.connect([this]() {
        this->windows->layoutChannelViews();
    });

    this->twitch.server = this->twitch2;
    this->twitch.pubsub = this->twitch2->pubsub;
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
    this->initPubsub();
}

int Application::run(QApplication &qtApp)
{
    assert(isAppInitialized);

    this->twitch.server->connect();

    if (!getArgs().isFramelessEmbed)
    {
        this->windows->getMainWindow().show();
    }

    getSettings()->betaUpdates.connect(
        [] {
            Updates::instance().checkForUpdates();
        },
        false);
    getSettings()->moderationActions.delayedItemsChanged.connect([this] {
        this->windows->forceLayoutChannelViews();
    });

    getSettings()->highlightedMessages.delayedItemsChanged.connect([this] {
        this->windows->forceLayoutChannelViews();
    });
    getSettings()->highlightedUsers.delayedItemsChanged.connect([this] {
        this->windows->forceLayoutChannelViews();
    });

    getSettings()->removeSpacesBetweenEmotes.connect([this] {
        this->windows->forceLayoutChannelViews();
    });

    return qtApp.exec();
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
#    if defined QT_NO_DEBUG || defined C_DEBUG_NM
    registerNmHost(paths);
    this->nmServer.start();
#    endif
#endif
}

void Application::initPubsub()
{
    this->twitch.pubsub->signals_.moderation.chatCleared.connect(
        [this](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);
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

    this->twitch.pubsub->signals_.moderation.modeChanged.connect(
        [this](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);
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

    this->twitch.pubsub->signals_.moderation.moderationStateChanged.connect(
        [this](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);
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

    this->twitch.pubsub->signals_.moderation.userBanned.connect(
        [&](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            MessageBuilder msg(action);
            msg->flags.set(MessageFlag::PubSub);

            postToThread([chan, msg = msg.release()] {
                chan->addOrReplaceTimeout(msg);
            });
        });
    this->twitch.pubsub->signals_.moderation.messageDeleted.connect(
        [&](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
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

    this->twitch.pubsub->signals_.moderation.userUnbanned.connect(
        [&](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            auto msg = MessageBuilder(action).release();

            postToThread([chan, msg] {
                chan->addMessage(msg);
            });
        });

    this->twitch.pubsub->signals_.moderation.automodMessage.connect(
        [&](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);

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

    this->twitch.pubsub->signals_.moderation.automodUserMessage.connect(
        [&](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);

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

    this->twitch.pubsub->signals_.moderation.automodInfoMessage.connect(
        [&](const auto &action) {
            auto chan =
                this->twitch.server->getChannelOrEmptyByID(action.roomID);

            if (chan->isEmpty())
            {
                return;
            }

            postToThread([chan, action] {
                const auto p = makeAutomodInfoMessage(action);
                chan->addMessage(p);
            });
        });

    this->twitch.pubsub->signals_.pointReward.redeemed.connect([&](auto &data) {
        QString channelId;
        if (rj::getSafe(data, "channel_id", channelId))
        {
            auto chan = this->twitch.server->getChannelOrEmptyByID(channelId);

            auto reward = ChannelPointReward(data);

            postToThread([chan, reward] {
                if (auto channel = dynamic_cast<TwitchChannel *>(chan.get()))
                {
                    channel->addChannelPointReward(reward);
                }
            });
        }
        else
        {
            qCDebug(chatterinoApp)
                << "Couldn't find channel id of point reward";
        }
    });

    this->twitch.pubsub->start();

    auto RequestModerationActions = [=]() {
        this->twitch.server->pubsub->unlistenAllModerationActions();
        // TODO(pajlada): Unlisten to all authed topics instead of only
        // moderation topics this->twitch.pubsub->UnlistenAllAuthedTopics();

        this->twitch.server->pubsub->listenToWhispers(
            this->accounts->twitch.getCurrent());
    };

    this->accounts->twitch.currentUserChanged.connect(RequestModerationActions);

    RequestModerationActions();
}

Application *getApp()
{
    assert(Application::instance != nullptr);

    assertInGuiThread();

    return Application::instance;
}

}  // namespace chatterino
