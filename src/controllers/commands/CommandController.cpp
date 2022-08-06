#include "CommandController.hpp"

#include "Application.hpp"
#include "common/Env.hpp"
#include "common/SignalVector.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/CombinePath.hpp"
#include "util/FormatTime.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/Qt.hpp"
#include "util/StreamLink.hpp"
#include "util/Twitch.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/ReplyThreadPopup.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/splits/Split.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

namespace {
using namespace chatterino;

void sendWhisperMessage(const QString &text)
{
    // (hemirt) pajlada: "we should not be sending whispers through jtv, but
    // rather to your own username"
    auto app = getApp();
    QString toSend = text.simplified();

    // This is to make sure that combined emoji go through properly, see
    // https://github.com/Chatterino/chatterino2/issues/3384 and
    // https://mm2pl.github.io/emoji_rfc.pdf for more details
    // Constants used here are defined in TwitchChannel.hpp
    toSend.replace(ZERO_WIDTH_JOINER, ESCAPE_TAG);

    app->twitch->sendMessage("jtv", toSend);
}

bool appendWhisperMessageWordsLocally(const QStringList &words)
{
    auto app = getApp();

    MessageBuilder b;

    b.emplace<TimestampElement>();
    b.emplace<TextElement>(app->accounts->twitch.getCurrent()->getUserName(),
                           MessageElementFlag::Text, MessageColor::Text,
                           FontStyle::ChatMediumBold);
    b.emplace<TextElement>("->", MessageElementFlag::Text,
                           getApp()->themes->messages.textColors.system);
    b.emplace<TextElement>(words[1] + ":", MessageElementFlag::Text,
                           MessageColor::Text, FontStyle::ChatMediumBold);

    const auto &acc = app->accounts->twitch.getCurrent();
    const auto &accemotes = *acc->accessEmotes();
    const auto &bttvemotes = app->twitch->getBttvEmotes();
    const auto &ffzemotes = app->twitch->getFfzEmotes();
    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};
    for (int i = 2; i < words.length(); i++)
    {
        {  // Twitch emote
            auto it = accemotes.emotes.find({words[i]});
            if (it != accemotes.emotes.end())
            {
                b.emplace<EmoteElement>(it->second,
                                        MessageElementFlag::TwitchEmote);
                continue;
            }
        }  // Twitch emote

        {  // bttv/ffz emote
            if ((emote = bttvemotes.emote({words[i]})))
            {
                flags = MessageElementFlag::BttvEmote;
            }
            else if ((emote = ffzemotes.emote({words[i]})))
            {
                flags = MessageElementFlag::FfzEmote;
            }
            if (emote)
            {
                b.emplace<EmoteElement>(emote.get(), flags);
                continue;
            }
        }  // bttv/ffz emote
        {  // emoji/text
            for (auto &variant : app->emotes->emojis.parse(words[i]))
            {
                constexpr const static struct {
                    void operator()(EmotePtr emote, MessageBuilder &b) const
                    {
                        b.emplace<EmoteElement>(emote,
                                                MessageElementFlag::EmojiAll);
                    }
                    void operator()(const QString &string,
                                    MessageBuilder &b) const
                    {
                        auto linkString = b.matchLink(string);
                        if (linkString.isEmpty())
                        {
                            b.emplace<TextElement>(string,
                                                   MessageElementFlag::Text);
                        }
                        else
                        {
                            b.addLink(string, linkString);
                        }
                    }
                } visitor;
                boost::apply_visitor(
                    [&b](auto &&arg) {
                        visitor(arg, b);
                    },
                    variant);
            }  // emoji/text
        }
    }

    b->flags.set(MessageFlag::DoNotTriggerNotification);
    b->flags.set(MessageFlag::Whisper);
    auto messagexD = b.release();

    app->twitch->whispersChannel->addMessage(messagexD);

    auto overrideFlags = boost::optional<MessageFlags>(messagexD->flags);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers)
    {
        app->twitch->forEachChannel(
            [&messagexD, overrideFlags](ChannelPtr _channel) {
                _channel->addMessage(messagexD, overrideFlags);
            });
    }

    return true;
}

bool appendWhisperMessageStringLocally(const QString &textNoEmoji)
{
    QString text = getApp()->emotes->emojis.replaceShortCodes(textNoEmoji);
    QStringList words = text.split(' ', Qt::SkipEmptyParts);

    if (words.length() == 0)
    {
        return false;
    }

    QString commandName = words[0];

    if (TWITCH_WHISPER_COMMANDS.contains(commandName, Qt::CaseInsensitive))
    {
        if (words.length() > 2)
        {
            return appendWhisperMessageWordsLocally(words);
        }
    }
    return false;
}

using VariableReplacer = std::function<QString(
    const QString &, const ChannelPtr &, const Message *)>;

const VariableReplacer NO_OP_PLACEHOLDER =
    [](const auto &altText, const auto &channel, const auto *message) {
        return altText;
    };

const std::unordered_map<QString, VariableReplacer> COMMAND_VARS{
    {
        "channel.name",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(altText);  //unused
            (void)(message);  //unused
            return channel->getName();
        },
    },
    {
        "channel.id",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(message);  //unused
            auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
            if (tc == nullptr)
            {
                return altText;
            }

            return tc->roomId();
        },
    },
    {
        // NOTE: The use of {channel} is deprecated and support for it will drop at some point
        // Users should be encouraged to use {channel.name} instead.
        "channel",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(altText);  //unused
            (void)(message);  //unused
            return channel->getName();
        },
    },
    {
        "stream.game",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(message);  //unused
            auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
            if (tc == nullptr)
            {
                return altText;
            }
            const auto &status = tc->accessStreamStatus();
            return status->live ? status->game : altText;
        },
    },
    {
        "stream.title",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(message);  //unused
            auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
            if (tc == nullptr)
            {
                return altText;
            }
            const auto &status = tc->accessStreamStatus();
            return status->live ? status->title : altText;
        },
    },
    {
        "my.id",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            (void)(message);  //unused
            auto uid = getApp()->accounts->twitch.getCurrent()->getUserId();
            return uid.isEmpty() ? altText : uid;
        },
    },
    {
        "my.name",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            (void)(message);  //unused
            auto name = getApp()->accounts->twitch.getCurrent()->getUserName();
            return name.isEmpty() ? altText : name;
        },
    },
    {
        "user.name",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            if (message == nullptr)
            {
                return altText;
            }

            const auto &v = message->loginName;

            if (v.isEmpty())
            {
                return altText;
            }

            return v;
        },
    },
    {
        // NOTE: The use of {user} is deprecated and support for it will drop at some point
        // Users should be encouraged to use {user.name} instead.
        "user",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            if (message == nullptr)
            {
                return altText;
            }

            const auto &v = message->loginName;

            if (v.isEmpty())
            {
                return altText;
            }

            return v;
        },
    },
    {
        "msg.id",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            if (message == nullptr)
            {
                return altText;
            }

            const auto &v = message->id;

            if (v.isEmpty())
            {
                return altText;
            }

            return v;
        },
    },
    {
        // NOTE: The use of {msg-id} is deprecated and support for it will drop at some point
        // Users should be encouraged to use {msg.id} instead.
        "msg-id",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            if (message == nullptr)
            {
                return altText;
            }

            const auto &v = message->id;

            if (v.isEmpty())
            {
                return altText;
            }

            return v;
        },
    },
    {
        "msg.text",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            if (message == nullptr)
            {
                return altText;
            }

            const auto &v = message->messageText;

            if (v.isEmpty())
            {
                return altText;
            }

            return v;
        },
    },
    {
        // NOTE: The use of {message} is deprecated and support for it will drop at some point
        // Users should be encouraged to use {msg.text} instead.
        "message",
        [](const auto &altText, const auto &channel, const auto *message) {
            (void)(channel);  //unused
            if (message == nullptr)
            {
                return altText;
            }

            const auto &v = message->messageText;

            if (v.isEmpty())
            {
                return altText;
            }

            return v;
        },
    },
    // variables used in mod buttons and the like, these make no sense in normal commands, so they are left empty
    {"input.text", NO_OP_PLACEHOLDER},
};

}  // namespace

namespace chatterino {

void CommandController::initialize(Settings &, Paths &paths)
{
    // Update commands map when the vector of commands has been updated
    auto addFirstMatchToMap = [this](auto args) {
        this->userCommands_.remove(args.item.name);

        for (const Command &cmd : this->items)
        {
            if (cmd.name == args.item.name)
            {
                this->userCommands_[cmd.name] = cmd;
                break;
            }
        }

        int maxSpaces = 0;

        for (const Command &cmd : this->items)
        {
            auto localMaxSpaces = cmd.name.count(' ');
            if (localMaxSpaces > maxSpaces)
            {
                maxSpaces = localMaxSpaces;
            }
        }

        this->maxSpaces_ = maxSpaces;
    };
    this->items.itemInserted.connect(addFirstMatchToMap);
    this->items.itemRemoved.connect(addFirstMatchToMap);

    // Initialize setting manager for commands.json
    auto path = combinePath(paths.settingsDirectory, "commands.json");
    this->sm_ = std::make_shared<pajlada::Settings::SettingManager>();
    this->sm_->setPath(qPrintable(path));
    this->sm_->setBackupEnabled(true);
    this->sm_->setBackupSlots(9);

    // Delayed initialization of the setting storing all commands
    this->commandsSetting_.reset(
        new pajlada::Settings::Setting<std::vector<Command>>("/commands",
                                                             this->sm_));

    // Update the setting when the vector of commands has been updated (most
    // likely from the settings dialog)
    this->items.delayedItemsChanged.connect([this] {
        this->commandsSetting_->setValue(this->items.raw());
    });

    // Load commands from commands.json
    this->sm_->load();

    // Add loaded commands to our vector of commands (which will update the map
    // of commands)
    for (const auto &command : this->commandsSetting_->getValue())
    {
        this->items.append(command);
    }

    /// Deprecated commands

    auto blockLambda = [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /block <user>"));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to block someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [currentUser, channel, target](const HelixUser &targetUser) {
                getApp()->accounts->twitch.getCurrent()->blockUser(
                    targetUser.id,
                    [channel, target, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString("You successfully blocked user %1")
                                .arg(target)));
                    },
                    [channel, target] {
                        channel->addMessage(makeSystemMessage(
                            QString("User %1 couldn't be blocked, an unknown "
                                    "error occurred!")
                                .arg(target)));
                    });
            },
            [channel, target] {
                channel->addMessage(
                    makeSystemMessage(QString("User %1 couldn't be blocked, no "
                                              "user with that name found!")
                                          .arg(target)));
            });

        return "";
    };

    auto unblockLambda = [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /unblock <user>"));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();

        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to unblock someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [currentUser, channel, target](const auto &targetUser) {
                getApp()->accounts->twitch.getCurrent()->unblockUser(
                    targetUser.id,
                    [channel, target, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString("You successfully unblocked user %1")
                                .arg(target)));
                    },
                    [channel, target] {
                        channel->addMessage(makeSystemMessage(
                            QString("User %1 couldn't be unblocked, an unknown "
                                    "error occurred!")
                                .arg(target)));
                    });
            },
            [channel, target] {
                channel->addMessage(
                    makeSystemMessage(QString("User %1 couldn't be unblocked, "
                                              "no user with that name found!")
                                          .arg(target)));
            });

        return "";
    };

    this->registerCommand(
        "/ignore", [blockLambda](const auto &words, auto channel) {
            channel->addMessage(makeSystemMessage(
                "Ignore command has been renamed to /block, please use it from "
                "now on as /ignore is going to be removed soon."));
            blockLambda(words, channel);
            return "";
        });

    this->registerCommand(
        "/unignore", [unblockLambda](const auto &words, auto channel) {
            channel->addMessage(makeSystemMessage(
                "Unignore command has been renamed to /unblock, please use it "
                "from now on as /unignore is going to be removed soon."));
            unblockLambda(words, channel);
            return "";
        });

    this->registerCommand("/follow", [](const auto &words, auto channel) {
        channel->addMessage(makeSystemMessage(
            "Twitch has removed the ability to follow users through "
            "third-party applications. For more information, see "
            "https://github.com/Chatterino/chatterino2/issues/3076"));
        return "";
    });

    this->registerCommand("/unfollow", [](const auto &words, auto channel) {
        channel->addMessage(makeSystemMessage(
            "Twitch has removed the ability to unfollow users through "
            "third-party applications. For more information, see "
            "https://github.com/Chatterino/chatterino2/issues/3076"));
        return "";
    });

    /// Supported commands

    this->registerCommand(
        "/debug-args", [](const auto & /*words*/, auto channel) {
            QString msg = QApplication::instance()->arguments().join(' ');

            channel->addMessage(makeSystemMessage(msg));

            return "";
        });

    this->registerCommand("/debug-env", [](const auto & /*words*/,
                                           ChannelPtr channel) {
        auto env = Env::get();

        QStringList debugMessages{
            "recentMessagesApiUrl: " + env.recentMessagesApiUrl,
            "linkResolverUrl: " + env.linkResolverUrl,
            "twitchServerHost: " + env.twitchServerHost,
            "twitchServerPort: " + QString::number(env.twitchServerPort),
            "twitchServerSecure: " + QString::number(env.twitchServerSecure),
        };

        for (QString &str : debugMessages)
        {
            MessageBuilder builder;
            builder.emplace<TimestampElement>(QTime::currentTime());
            builder.emplace<TextElement>(str, MessageElementFlag::Text,
                                         MessageColor::System);
            channel->addMessage(builder.release());
        }
        return "";
    });

    this->registerCommand("/uptime", [](const auto & /*words*/, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /uptime command only works in Twitch Channels"));
            return "";
        }

        const auto &streamStatus = twitchChannel->accessStreamStatus();

        QString messageText =
            streamStatus->live ? streamStatus->uptime : "Channel is not live.";

        channel->addMessage(makeSystemMessage(messageText));

        return "";
    });

    this->registerCommand("/block", blockLambda);

    this->registerCommand("/unblock", unblockLambda);

    this->registerCommand("/user", [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /user <user> [channel]"));
            return "";
        }
        QString userName = words[1];
        stripUserName(userName);

        QString channelName = channel->getName();

        if (words.size() > 2)
        {
            channelName = words[2];
            stripChannelName(channelName);
        }
        openTwitchUsercard(channelName, userName);

        return "";
    });

    this->registerCommand("/usercard", [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /usercard <user> [channel]"));
            return "";
        }

        QString userName = words[1];
        stripUserName(userName);

        if (words.size() > 2)
        {
            QString channelName = words[2];
            stripChannelName(channelName);

            ChannelPtr channelTemp =
                getApp()->twitch->getChannelOrEmpty(channelName);

            if (channelTemp->isEmpty())
            {
                channel->addMessage(makeSystemMessage(
                    "A usercard can only be displayed for a channel that is "
                    "currently opened in Chatterino."));
                return "";
            }

            channel = channelTemp;
        }

        auto *userPopup = new UserInfoPopup(
            getSettings()->autoCloseUserPopup,
            static_cast<QWidget *>(&(getApp()->windows->getMainWindow())),
            nullptr);
        userPopup->setData(userName, channel);
        userPopup->move(QCursor::pos());
        userPopup->show();
        return "";
    });

    this->registerCommand("/requests", [](const QStringList &words,
                                          ChannelPtr channel) {
        QString target(words.value(1));

        if (target.isEmpty())
        {
            if (channel->getType() == Channel::Type::Twitch &&
                !channel->isEmpty())
            {
                target = channel->getName();
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: /requests [channel]. You can also use the command "
                    "without arguments in any Twitch channel to open its "
                    "channel points requests queue. Only the broadcaster and "
                    "moderators have permission to view the queue."));
                return "";
            }
        }

        stripChannelName(target);
        QDesktopServices::openUrl(
            QUrl(QString("https://www.twitch.tv/popout/%1/reward-queue")
                     .arg(target)));

        return "";
    });

    this->registerCommand(
        "/chatters", [](const auto & /*words*/, auto channel) {
            auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /chatters command only works in Twitch Channels"));
                return "";
            }

            channel->addMessage(makeSystemMessage(
                QString("Chatter count: %1")
                    .arg(localizeNumbers(twitchChannel->chatterCount()))));

            return "";
        });

    this->registerCommand("/clip", [](const auto & /*words*/, auto channel) {
        if (const auto type = channel->getType();
            type != Channel::Type::Twitch &&
            type != Channel::Type::TwitchWatching)
        {
            return "";
        }

        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

        twitchChannel->createClip();

        return "";
    });

    this->registerCommand("/marker", [](const QStringList &words,
                                        auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /marker command only works in Twitch channels"));
            return "";
        }

        // Avoid Helix calls without Client ID and/or OAuth Token
        if (getApp()->accounts->twitch.getCurrent()->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You need to be logged in to create stream markers!"));
            return "";
        }

        // Exact same message as in webchat
        if (!twitchChannel->isLive())
        {
            channel->addMessage(makeSystemMessage(
                "You can only add stream markers during live streams. Try "
                "again when the channel is live streaming."));
            return "";
        }

        auto arguments = words;
        arguments.removeFirst();

        getHelix()->createStreamMarker(
            // Limit for description is 140 characters, webchat just crops description
            // if it's >140 characters, so we're doing the same thing
            twitchChannel->roomId(), arguments.join(" ").left(140),
            [channel, arguments](const HelixStreamMarker &streamMarker) {
                channel->addMessage(makeSystemMessage(
                    QString("Successfully added a stream marker at %1%2")
                        .arg(formatTime(streamMarker.positionSeconds))
                        .arg(streamMarker.description.isEmpty()
                                 ? ""
                                 : QString(": \"%1\"")
                                       .arg(streamMarker.description))));
            },
            [channel](auto error) {
                QString errorMessage("Failed to create stream marker - ");

                switch (error)
                {
                    case HelixStreamMarkerError::UserNotAuthorized: {
                        errorMessage +=
                            "you don't have permission to perform that action.";
                    }
                    break;

                    case HelixStreamMarkerError::UserNotAuthenticated: {
                        errorMessage += "you need to re-authenticate.";
                    }
                    break;

                    // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                    case HelixStreamMarkerError::Unknown:
                    default: {
                        errorMessage += "an unknown error occurred.";
                    }
                    break;
                }

                channel->addMessage(makeSystemMessage(errorMessage));
            });

        return "";
    });

    this->registerCommand("/streamlink", [](const QStringList &words,
                                            ChannelPtr channel) {
        QString target(words.value(1));

        if (target.isEmpty())
        {
            if (channel->getType() == Channel::Type::Twitch &&
                !channel->isEmpty())
            {
                target = channel->getName();
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    "/streamlink [channel]. Open specified Twitch channel in "
                    "streamlink. If no channel argument is specified, open the "
                    "current Twitch channel instead."));
                return "";
            }
        }

        stripChannelName(target);
        openStreamlinkForChannel(target);

        return "";
    });

    this->registerCommand("/popout", [](const QStringList &words,
                                        ChannelPtr channel) {
        QString target(words.value(1));

        if (target.isEmpty())
        {
            if (channel->getType() == Channel::Type::Twitch &&
                !channel->isEmpty())
            {
                target = channel->getName();
            }
            else
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: /popout <channel>. You can also use the command "
                    "without arguments in any Twitch channel to open its "
                    "popout chat."));
                return "";
            }
        }

        stripChannelName(target);
        QDesktopServices::openUrl(
            QUrl(QString("https://www.twitch.tv/popout/%1/chat?popout=")
                     .arg(target)));

        return "";
    });

    this->registerCommand("/popup", [](const QStringList &words,
                                       ChannelPtr sourceChannel) {
        static const auto *usageMessage =
            "Usage: /popup [channel]. Open specified Twitch channel in "
            "a new window. If no channel argument is specified, open "
            "the currently selected split instead.";

        QString target(words.value(1));
        stripChannelName(target);

        // Popup the current split
        if (target.isEmpty())
        {
            auto *currentPage =
                dynamic_cast<SplitContainer *>(getApp()
                                                   ->windows->getMainWindow()
                                                   .getNotebook()
                                                   .getSelectedPage());
            if (currentPage != nullptr)
            {
                auto *currentSplit = currentPage->getSelectedSplit();
                if (currentSplit != nullptr)
                {
                    currentSplit->popup();

                    return "";
                }
            }

            sourceChannel->addMessage(makeSystemMessage(usageMessage));
            return "";
        }

        // Open channel passed as argument in a popup
        auto *app = getApp();
        auto targetChannel = app->twitch->getOrAddChannel(target);
        app->windows->openInPopup(targetChannel);

        return "";
    });

    this->registerCommand("/clearmessages", [](const auto & /*words*/,
                                               ChannelPtr channel) {
        auto *currentPage = dynamic_cast<SplitContainer *>(
            getApp()->windows->getMainWindow().getNotebook().getSelectedPage());

        if (auto split = currentPage->getSelectedSplit())
        {
            split->getChannelView().clearMessages();
        }

        return "";
    });

    this->registerCommand("/settitle", [](const QStringList &words,
                                          ChannelPtr channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /settitle <stream title>"));
            return "";
        }
        if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
        {
            auto status = twitchChannel->accessStreamStatus();
            auto title = words.mid(1).join(" ");
            getHelix()->updateChannel(
                twitchChannel->roomId(), "", "", title,
                [channel, title](NetworkResult) {
                    channel->addMessage(makeSystemMessage(
                        QString("Updated title to %1").arg(title)));
                },
                [channel] {
                    channel->addMessage(
                        makeSystemMessage("Title update failed! Are you "
                                          "missing the required scope?"));
                });
        }
        else
        {
            channel->addMessage(makeSystemMessage(
                "Unable to set title of non-Twitch channel."));
        }
        return "";
    });

    this->registerCommand("/setgame", [](const QStringList &words,
                                         const ChannelPtr channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /setgame <stream game>"));
            return "";
        }
        if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
        {
            const auto gameName = words.mid(1).join(" ");

            getHelix()->searchGames(
                gameName,
                [channel, twitchChannel,
                 gameName](const std::vector<HelixGame> &games) {
                    if (games.empty())
                    {
                        channel->addMessage(
                            makeSystemMessage("Game not found."));
                        return;
                    }

                    auto matchedGame = games.at(0);

                    if (games.size() > 1)
                    {
                        // NOTE: Improvements could be made with 'fuzzy string matching' code here
                        // attempt to find the best looking game by comparing exactly with lowercase values
                        for (const auto &game : games)
                        {
                            if (game.name.toLower() == gameName.toLower())
                            {
                                matchedGame = game;
                                break;
                            }
                        }
                    }

                    auto status = twitchChannel->accessStreamStatus();
                    getHelix()->updateChannel(
                        twitchChannel->roomId(), matchedGame.id, "", "",
                        [channel, games, matchedGame](const NetworkResult &) {
                            channel->addMessage(
                                makeSystemMessage(QString("Updated game to %1")
                                                      .arg(matchedGame.name)));
                        },
                        [channel] {
                            channel->addMessage(makeSystemMessage(
                                "Game update failed! Are you "
                                "missing the required scope?"));
                        });
                },
                [channel] {
                    channel->addMessage(
                        makeSystemMessage("Failed to look up game."));
                });
        }
        else
        {
            channel->addMessage(
                makeSystemMessage("Unable to set game of non-Twitch channel."));
        }
        return "";
    });

    this->registerCommand("/openurl", [](const QStringList &words,
                                         const ChannelPtr channel) {
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage("Usage: /openurl <URL>"));
            return "";
        }

        QUrl url = QUrl::fromUserInput(words.mid(1).join(" "));
        if (!url.isValid())
        {
            channel->addMessage(makeSystemMessage("Invalid URL specified."));
            return "";
        }

        bool res = false;
        if (supportsIncognitoLinks() && getSettings()->openLinksIncognito)
        {
            res = openLinkIncognito(url.toString(QUrl::FullyEncoded));
        }
        else
        {
            res = QDesktopServices::openUrl(url);
        }

        if (!res)
        {
            channel->addMessage(makeSystemMessage("Could not open URL."));
        }

        return "";
    });

    this->registerCommand(
        "/delete", [](const QStringList &words, ChannelPtr channel) -> QString {
            // This is a wrapper over the standard Twitch /delete command
            // We use this to ensure the user gets better error messages for missing or malformed arguments
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /delete <msg-id> - Deletes the "
                                      "specified message."));
                return "";
            }

            auto messageID = words.at(1);
            auto uuid = QUuid(messageID);
            if (uuid.isNull())
            {
                // The message id must be a valid UUID
                channel->addMessage(makeSystemMessage(
                    QString("Invalid msg-id: \"%1\"").arg(messageID)));
                return "";
            }

            auto msg = channel->findMessage(messageID);
            if (msg != nullptr)
            {
                if (msg->loginName == channel->getName() &&
                    !channel->isBroadcaster())
                {
                    channel->addMessage(makeSystemMessage(
                        "You cannot delete the broadcaster's messages unless "
                        "you are the broadcaster."));
                    return "";
                }
            }

            return QString("/delete ") + messageID;
        });

    this->registerCommand("/raw", [](const QStringList &words, ChannelPtr) {
        getApp()->twitch->sendRawMessage(words.mid(1).join(" "));
        return "";
    });

    this->registerCommand(
        "/reply", [](const QStringList &words, ChannelPtr channel) {
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /reply command only works in Twitch channels"));
                return "";
            }

            if (words.size() < 3)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /reply <username> <message>"));
                return "";
            }

            QString username = words[1];
            stripChannelName(username);

            auto snapshot = twitchChannel->getMessageSnapshot();
            for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it)
            {
                const auto &msg = *it;
                if (msg->loginName.compare(username, Qt::CaseInsensitive) == 0)
                {
                    std::shared_ptr<MessageThread> thread;
                    // found most recent message by user
                    if (msg->replyThread == nullptr)
                    {
                        thread = std::make_shared<MessageThread>(msg);
                        twitchChannel->addReplyThread(thread);
                    }
                    else
                    {
                        thread = msg->replyThread;
                    }

                    QString reply = words.mid(2).join(" ");
                    twitchChannel->sendReply(reply, thread->rootId());
                    return "";
                }
            }

            channel->addMessage(
                makeSystemMessage("A message from that user wasn't found"));

            return "";
        });

#ifndef NDEBUG
    this->registerCommand(
        "/fakemsg",
        [](const QStringList &words, ChannelPtr channel) -> QString {
            if (words.size() < 2)
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: /fakemsg (raw irc text) - injects raw irc text as "
                    "if it was a message received from TMI"));
                return "";
            }
            auto ircText = words.mid(1).join(" ");
            getApp()->twitch->addFakeMessage(ircText);
            return "";
        });
#endif

    this->registerCommand(
        "/copy", [](const QStringList &words, ChannelPtr channel) -> QString {
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /copy <text> - copies provided "
                                      "text to clipboard."));
                return "";
            }
            crossPlatformCopy(words.mid(1).join(" "));
            return "";
        });
}

void CommandController::save()
{
    this->sm_->save();
}

CommandModel *CommandController::createModel(QObject *parent)
{
    CommandModel *model = new CommandModel(parent);
    model->initialize(&this->items);

    return model;
}

QString CommandController::execCommand(const QString &textNoEmoji,
                                       ChannelPtr channel, bool dryRun)
{
    QString text = getApp()->emotes->emojis.replaceShortCodes(textNoEmoji);
    QStringList words = text.split(' ', Qt::SkipEmptyParts);

    if (words.length() == 0)
    {
        return text;
    }

    QString commandName = words[0];

    // works in a valid Twitch channel and /whispers, etc...
    if (!dryRun && channel->isTwitchChannel())
    {
        if (TWITCH_WHISPER_COMMANDS.contains(commandName, Qt::CaseInsensitive))
        {
            if (words.length() > 2)
            {
                appendWhisperMessageWordsLocally(words);
                sendWhisperMessage(text);
            }
            else
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /w <username> <message>"));
            }

            return "";
        }
    }

    {
        // check if user command exists
        const auto it = this->userCommands_.find(commandName);
        if (it != this->userCommands_.end())
        {
            text = getApp()->emotes->emojis.replaceShortCodes(
                this->execCustomCommand(words, it.value(), dryRun, channel));

            words = text.split(' ', Qt::SkipEmptyParts);

            if (words.length() == 0)
            {
                return text;
            }

            commandName = words[0];
        }
    }

    // works only in a valid Twitch channel
    if (!dryRun && channel->isTwitchChannel())
    {
        // check if command exists
        const auto it = this->commands_.find(commandName);
        if (it != this->commands_.end())
        {
            return it.value()(words, channel);
        }
    }

    auto maxSpaces = std::min(this->maxSpaces_, words.length() - 1);
    for (int i = 0; i < maxSpaces; ++i)
    {
        commandName += ' ' + words[i + 1];

        const auto it = this->userCommands_.find(commandName);
        if (it != this->userCommands_.end())
        {
            return this->execCustomCommand(words, it.value(), dryRun, channel);
        }
    }

    if (!dryRun && channel->getType() == Channel::Type::TwitchWhispers)
    {
        channel->addMessage(
            makeSystemMessage("Use /w <username> <message> to whisper"));
        return "";
    }

    return text;
}

void CommandController::registerCommand(QString commandName,
                                        CommandFunction commandFunction)
{
    assert(!this->commands_.contains(commandName));

    this->commands_[commandName] = commandFunction;

    this->defaultChatterinoCommandAutoCompletions_.append(commandName);
}

QString CommandController::execCustomCommand(
    const QStringList &words, const Command &command, bool dryRun,
    ChannelPtr channel, const Message *message,
    std::unordered_map<QString, QString> context)
{
    QString result;

    static QRegularExpression parseCommand(
        R"((^|[^{])({{)*{(\d+\+?|([a-zA-Z.-]+)(?:;(.+?))?)})");

    int lastCaptureEnd = 0;

    auto globalMatch = parseCommand.globalMatch(command.func);
    int matchOffset = 0;

    while (true)
    {
        QRegularExpressionMatch match =
            parseCommand.match(command.func, matchOffset);

        if (!match.hasMatch())
        {
            break;
        }

        result += command.func.mid(lastCaptureEnd,
                                   match.capturedStart() - lastCaptureEnd + 1);

        lastCaptureEnd = match.capturedEnd();
        matchOffset = lastCaptureEnd - 1;

        QString wordIndexMatch = match.captured(3);

        bool plus = wordIndexMatch.at(wordIndexMatch.size() - 1) == '+';
        wordIndexMatch = wordIndexMatch.replace("+", "");

        bool ok;
        int wordIndex = wordIndexMatch.replace("=", "").toInt(&ok);
        if (!ok || wordIndex == 0)
        {
            auto varName = match.captured(4);
            auto altText = match.captured(5);  // alt text or empty string

            auto var = context.find(varName);

            if (var != context.end())
            {
                // Found variable in `context`
                result += var->second.isEmpty() ? altText : var->second;
                continue;
            }

            auto it = COMMAND_VARS.find(varName);
            if (it != COMMAND_VARS.end())
            {
                // Found variable in `COMMAND_VARS`
                result += it->second(altText, channel, message);
                continue;
            }

            // Fall back to replacing it with the actual matched string
            result += "{" + match.captured(3) + "}";
            continue;
        }

        if (words.length() <= wordIndex)
        {
            continue;
        }

        if (plus)
        {
            bool first = true;
            for (int i = wordIndex; i < words.length(); i++)
            {
                if (!first)
                {
                    result += " ";
                }
                result += words[i];
                first = false;
            }
        }
        else
        {
            result += words[wordIndex];
        }
    }

    result += command.func.mid(lastCaptureEnd);

    if (result.size() > 0 && result.at(0) == '{')
    {
        result = result.mid(1);
    }

    auto res = result.replace("{{", "{");

    if (dryRun || !appendWhisperMessageStringLocally(res))
    {
        return res;
    }
    else
    {
        sendWhisperMessage(res);
        return "";
    }
}

QStringList CommandController::getDefaultChatterinoCommandList()
{
    return this->defaultChatterinoCommandAutoCompletions_;
}

}  // namespace chatterino
