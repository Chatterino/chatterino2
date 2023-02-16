#include "controllers/commands/CommandController.hpp"

#include "Application.hpp"
#include "common/Env.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "common/SignalVector.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/builtin/twitch/ChatSettings.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "messages/MessageThread.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
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
#include "util/StreamerMode.hpp"
#include "util/StreamLink.hpp"
#include "util/Twitch.hpp"
#include "widgets/dialogs/ReplyThreadPopup.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

namespace {

using namespace chatterino;

bool areIRCCommandsStillAvailable()
{
    // 11th of February 2023, 06:00am UTC
    const QDateTime migrationTime(QDate(2023, 2, 11), QTime(6, 0), Qt::UTC);
    auto now = QDateTime::currentDateTimeUtc();
    return now < migrationTime;
}

QString useIRCCommand(const QStringList &words)
{
    // Reform the original command
    auto originalCommand = words.join(" ");

    // Replace the / with a . to pass it along to TMI
    auto newCommand = originalCommand;
    newCommand.replace(0, 1, ".");

    qCDebug(chatterinoTwitch)
        << "Forwarding command" << originalCommand << "as" << newCommand;

    return newCommand;
}

void sendWhisperMessage(const QString &text)
{
    // (hemirt) pajlada: "we should not be sending whispers through jtv, but
    // rather to your own username"
    auto app = getApp();
    QString toSend = text.simplified();

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

    if (getSettings()->inlineWhispers &&
        !(getSettings()->streamerModeSuppressInlineWhispers &&
          isInStreamerMode()))
    {
        app->twitch->forEachChannel(
            [&messagexD, overrideFlags](ChannelPtr _channel) {
                _channel->addMessage(messagexD, overrideFlags);
            });
    }

    return true;
}

bool useIrcForWhisperCommand()
{
    switch (getSettings()->helixTimegateWhisper.getValue())
    {
        case HelixTimegateOverride::Timegate: {
            if (areIRCCommandsStillAvailable())
            {
                return true;
            }

            // fall through to Helix logic
        }
        break;

        case HelixTimegateOverride::AlwaysUseIRC: {
            return true;
        }
        break;

        case HelixTimegateOverride::AlwaysUseHelix: {
            // do nothing and fall through to Helix logic
        }
        break;
    }
    return false;
}

QString runWhisperCommand(const QStringList &words, const ChannelPtr &channel)
{
    if (words.size() < 3)
    {
        channel->addMessage(
            makeSystemMessage("Usage: /w <username> <message>"));
        return "";
    }

    auto currentUser = getApp()->accounts->twitch.getCurrent();
    if (currentUser->isAnon())
    {
        channel->addMessage(
            makeSystemMessage("You must be logged in to send a whisper!"));
        return "";
    }
    auto target = words.at(1);
    stripChannelName(target);
    auto message = words.mid(2).join(' ');
    if (channel->isTwitchChannel())
    {
        // this covers all twitch channels and twitch-like channels
        if (useIrcForWhisperCommand())
        {
            appendWhisperMessageWordsLocally(words);
            sendWhisperMessage(words.join(' '));
            return "";
        }
        getHelix()->getUserByName(
            target,
            [channel, currentUser, target, message,
             words](const auto &targetUser) {
                getHelix()->sendWhisper(
                    currentUser->getUserId(), targetUser.id, message,
                    [words] {
                        appendWhisperMessageWordsLocally(words);
                    },
                    [channel, target, targetUser](auto error, auto message) {
                        using Error = HelixWhisperError;

                        QString errorMessage = "Failed to send whisper - ";

                        switch (error)
                        {
                            case Error::NoVerifiedPhone: {
                                errorMessage +=
                                    "Due to Twitch restrictions, you are now "
                                    "required to have a verified phone number "
                                    "to send whispers. You can add a phone "
                                    "number in Twitch settings. "
                                    "https://www.twitch.tv/settings/security";
                            };
                            break;

                            case Error::RecipientBlockedUser: {
                                errorMessage +=
                                    "The recipient doesn't allow whispers "
                                    "from strangers or you directly.";
                            };
                            break;

                            case Error::WhisperSelf: {
                                errorMessage += "You cannot whisper yourself.";
                            };
                            break;

                            case Error::Forwarded: {
                                errorMessage += message;
                            }
                            break;

                            case Error::Ratelimited: {
                                errorMessage +=
                                    "You may only whisper a maximum of 40 "
                                    "unique recipients per day. Within the "
                                    "per day limit, you may whisper a "
                                    "maximum of 3 whispers per second and "
                                    "a maximum of 100 whispers per minute.";
                            }
                            break;

                            case Error::UserMissingScope: {
                                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                errorMessage += "Missing required scope. "
                                                "Re-login with your "
                                                "account and try again.";
                            }
                            break;

                            case Error::UserNotAuthorized: {
                                // TODO(pajlada): Phrase MISSING_PERMISSION
                                errorMessage += "You don't have permission to "
                                                "perform that action.";
                            }
                            break;

                            case Error::Unknown: {
                                errorMessage +=
                                    "An unknown error has occurred.";
                            }
                            break;
                        }
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel] {
                channel->addMessage(
                    makeSystemMessage("No user matching that username."));
            });
        return "";
    }
    // we must be on IRC
    auto *ircChannel = dynamic_cast<IrcChannel *>(channel.get());
    if (ircChannel == nullptr)
    {
        // give up
        return "";
    }
    auto *server = ircChannel->server();
    server->sendWhisper(target, message);
    return "";
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
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /block command only works in Twitch channels"));
            return "";
        }
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
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /unblock command only works in Twitch channels"));
            return "";
        }
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
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            return "";
        }
        channel->addMessage(makeSystemMessage(
            "Twitch has removed the ability to follow users through "
            "third-party applications. For more information, see "
            "https://github.com/Chatterino/chatterino2/issues/3076"));
        return "";
    });

    this->registerCommand("/unfollow", [](const auto &words, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            return "";
        }
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

        // try to link to current split if possible
        Split *currentSplit = nullptr;
        auto *currentPage = dynamic_cast<SplitContainer *>(
            getApp()->windows->getMainWindow().getNotebook().getSelectedPage());
        if (currentPage != nullptr)
        {
            currentSplit = currentPage->getSelectedSplit();
        }

        auto differentChannel =
            currentSplit != nullptr && currentSplit->getChannel() != channel;
        if (differentChannel || currentSplit == nullptr)
        {
            // not possible to use current split, try searching for one
            const auto &notebook =
                getApp()->windows->getMainWindow().getNotebook();
            auto count = notebook.getPageCount();
            for (int i = 0; i < count; i++)
            {
                auto *page = notebook.getPageAt(i);
                auto *container = dynamic_cast<SplitContainer *>(page);
                assert(container != nullptr);
                for (auto *split : container->getSplits())
                {
                    if (split->getChannel() == channel)
                    {
                        currentSplit = split;
                        break;
                    }
                }
            }

            // This would have crashed either way.
            assert(currentSplit != nullptr &&
                   "something went HORRIBLY wrong with the /usercard "
                   "command. It couldn't find a split for a channel which "
                   "should be open.");
        }

        auto *userPopup = new UserInfoPopup(
            getSettings()->autoCloseUserPopup,
            static_cast<QWidget *>(&(getApp()->windows->getMainWindow())),
            currentSplit);
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

    auto formatChattersError = [](HelixGetChattersError error,
                                  QString message) {
        using Error = HelixGetChattersError;

        QString errorMessage = QString("Failed to get chatter count - ");

        switch (error)
        {
            case Error::Forwarded: {
                errorMessage += message;
            }
            break;

            case Error::UserMissingScope: {
                errorMessage += "Missing required scope. "
                                "Re-login with your "
                                "account and try again.";
            }
            break;

            case Error::UserNotAuthorized: {
                errorMessage += "You must have moderator permissions to "
                                "use this command.";
            }
            break;

            case Error::Unknown: {
                errorMessage += "An unknown error has occurred.";
            }
            break;
        }
        return errorMessage;
    };

    this->registerCommand(
        "/chatters", [formatChattersError](const auto &words, auto channel) {
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /chatters command only works in Twitch Channels"));
                return "";
            }

            // Refresh chatter list via helix api for mods
            getHelix()->getChatters(
                twitchChannel->roomId(),
                getApp()->accounts->twitch.getCurrent()->getUserId(), 1,
                [channel](auto result) {
                    channel->addMessage(makeSystemMessage(
                        QString("Chatter count: %1")
                            .arg(localizeNumbers(result.total))));
                },
                [channel, formatChattersError](auto error, auto message) {
                    auto errorMessage = formatChattersError(error, message);
                    channel->addMessage(makeSystemMessage(errorMessage));
                });

            return "";
        });

    this->registerCommand("/test-chatters", [formatChattersError](
                                                const auto & /*words*/,
                                                auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /test-chatters command only works in Twitch Channels"));
            return "";
        }

        getHelix()->getChatters(
            twitchChannel->roomId(),
            getApp()->accounts->twitch.getCurrent()->getUserId(), 5000,
            [channel, twitchChannel](auto result) {
                QStringList entries;
                for (const auto &username : result.chatters)
                {
                    entries << username;
                }

                QString prefix = "Chatters ";

                if (result.total > 5000)
                {
                    prefix += QString("(5000/%1):").arg(result.total);
                }
                else
                {
                    prefix += QString("(%1):").arg(result.total);
                }

                MessageBuilder builder;
                TwitchMessageBuilder::listOfUsersSystemMessage(
                    prefix, entries, twitchChannel, &builder);

                channel->addMessage(builder.release());
            },
            [channel, formatChattersError](auto error, auto message) {
                auto errorMessage = formatChattersError(error, message);
                channel->addMessage(makeSystemMessage(errorMessage));
            });

        return "";
    });

    auto formatModsError = [](HelixGetModeratorsError error, QString message) {
        using Error = HelixGetModeratorsError;

        QString errorMessage = QString("Failed to get moderators - ");

        switch (error)
        {
            case Error::Forwarded: {
                errorMessage += message;
            }
            break;

            case Error::UserMissingScope: {
                errorMessage += "Missing required scope. "
                                "Re-login with your "
                                "account and try again.";
            }
            break;

            case Error::UserNotAuthorized: {
                errorMessage +=
                    "Due to Twitch restrictions, "
                    "this command can only be used by the broadcaster. "
                    "To see the list of mods you must use the Twitch website.";
            }
            break;

            case Error::Unknown: {
                errorMessage += "An unknown error has occurred.";
            }
            break;
        }
        return errorMessage;
    };

    this->registerCommand(
        "/mods",
        [formatModsError](const QStringList &words, auto channel) -> QString {
            auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /mods command only works in Twitch Channels"));
                return "";
            }

            switch (getSettings()->helixTimegateModerators.getValue())
            {
                case HelixTimegateOverride::Timegate: {
                    if (areIRCCommandsStillAvailable())
                    {
                        return useIRCCommand(words);
                    }
                }
                break;

                case HelixTimegateOverride::AlwaysUseIRC: {
                    return useIRCCommand(words);
                }
                break;
                case HelixTimegateOverride::AlwaysUseHelix: {
                    // Fall through to helix logic
                }
                break;
            }

            getHelix()->getModerators(
                twitchChannel->roomId(), 500,
                [channel, twitchChannel](auto result) {
                    // TODO: sort results?

                    MessageBuilder builder;
                    TwitchMessageBuilder::listOfUsersSystemMessage(
                        "The moderators of this channel are", result,
                        twitchChannel, &builder);
                    channel->addMessage(builder.release());
                },
                [channel, formatModsError](auto error, auto message) {
                    auto errorMessage = formatModsError(error, message);
                    channel->addMessage(makeSystemMessage(errorMessage));
                });
            return "";
        });

    this->registerCommand("/clip", [](const auto & /*words*/, auto channel) {
        if (const auto type = channel->getType();
            type != Channel::Type::Twitch &&
            type != Channel::Type::TwitchWatching)
        {
            channel->addMessage(makeSystemMessage(
                "The /clip command only works in Twitch Channels"));
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
        "/raw", [](const QStringList &words, ChannelPtr channel) -> QString {
            if (channel->isTwitchChannel())
            {
                getApp()->twitch->sendRawMessage(words.mid(1).join(" "));
            }
            else
            {
                // other code down the road handles this for IRC
                return words.join(" ");
            }
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
            if (!channel->isTwitchChannel())
            {
                channel->addMessage(makeSystemMessage(
                    "The /fakemsg command only works in Twitch channels."));
                return "";
            }
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

    this->registerCommand("/color", [](const QStringList &words, auto channel) {
        if (!channel->isTwitchChannel())
        {
            channel->addMessage(makeSystemMessage(
                "The /color command only works in Twitch channels"));
            return "";
        }
        auto user = getApp()->accounts->twitch.getCurrent();

        // Avoid Helix calls without Client ID and/or OAuth Token
        if (user->isAnon())
        {
            channel->addMessage(makeSystemMessage(
                "You must be logged in to use the /color command"));
            return "";
        }

        auto colorString = words.value(1);

        if (colorString.isEmpty())
        {
            channel->addMessage(makeSystemMessage(
                QString("Usage: /color <color> - Color must be one of Twitch's "
                        "supported colors (%1) or a hex code (#000000) if you "
                        "have Turbo or Prime.")
                    .arg(VALID_HELIX_COLORS.join(", "))));
            return "";
        }

        cleanHelixColorName(colorString);

        getHelix()->updateUserChatColor(
            user->getUserId(), colorString,
            [colorString, channel] {
                QString successMessage =
                    QString("Your color has been changed to %1.")
                        .arg(colorString);
                channel->addMessage(makeSystemMessage(successMessage));
            },
            [colorString, channel](auto error, auto message) {
                QString errorMessage =
                    QString("Failed to change color to %1 - ").arg(colorString);

                switch (error)
                {
                    case HelixUpdateUserChatColorError::UserMissingScope: {
                        errorMessage +=
                            "Missing required scope. Re-login with your "
                            "account and try again.";
                    }
                    break;

                    case HelixUpdateUserChatColorError::InvalidColor: {
                        errorMessage += QString("Color must be one of Twitch's "
                                                "supported colors (%1) or a "
                                                "hex code (#000000) if you "
                                                "have Turbo or Prime.")
                                            .arg(VALID_HELIX_COLORS.join(", "));
                    }
                    break;

                    case HelixUpdateUserChatColorError::Forwarded: {
                        errorMessage += message + ".";
                    }
                    break;

                    case HelixUpdateUserChatColorError::Unknown:
                    default: {
                        errorMessage += "An unknown error has occurred.";
                    }
                    break;
                }

                channel->addMessage(makeSystemMessage(errorMessage));
            });

        return "";
    });

    auto deleteMessages = [](TwitchChannel *twitchChannel,
                             const QString &messageID) {
        const auto *commandName = messageID.isEmpty() ? "/clear" : "/delete";

        auto user = getApp()->accounts->twitch.getCurrent();

        // Avoid Helix calls without Client ID and/or OAuth Token
        if (user->isAnon())
        {
            twitchChannel->addMessage(makeSystemMessage(
                QString("You must be logged in to use the %1 command.")
                    .arg(commandName)));
            return "";
        }

        getHelix()->deleteChatMessages(
            twitchChannel->roomId(), user->getUserId(), messageID,
            []() {
                // Success handling, we do nothing: IRC/pubsub-edge will dispatch the correct
                // events to update state for us.
            },
            [twitchChannel, messageID](auto error, auto message) {
                QString errorMessage =
                    QString("Failed to delete chat messages - ");

                switch (error)
                {
                    case HelixDeleteChatMessagesError::UserMissingScope: {
                        errorMessage +=
                            "Missing required scope. Re-login with your "
                            "account and try again.";
                    }
                    break;

                    case HelixDeleteChatMessagesError::UserNotAuthorized: {
                        errorMessage +=
                            "you don't have permission to perform that action.";
                    }
                    break;

                    case HelixDeleteChatMessagesError::MessageUnavailable: {
                        // Override default message prefix to match with IRC message format
                        errorMessage =
                            QString(
                                "The message %1 does not exist, was deleted, "
                                "or is too old to be deleted.")
                                .arg(messageID);
                    }
                    break;

                    case HelixDeleteChatMessagesError::UserNotAuthenticated: {
                        errorMessage += "you need to re-authenticate.";
                    }
                    break;

                    case HelixDeleteChatMessagesError::Forwarded: {
                        errorMessage += message;
                    }
                    break;

                    case HelixDeleteChatMessagesError::Unknown:
                    default: {
                        errorMessage += "An unknown error has occurred.";
                    }
                    break;
                }

                twitchChannel->addMessage(makeSystemMessage(errorMessage));
            });

        return "";
    };

    this->registerCommand(
        "/clear", [deleteMessages](const QStringList &words, auto channel) {
            (void)words;  // unused
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /clear command only works in Twitch channels"));
                return "";
            }
            return deleteMessages(twitchChannel, QString());
        });

    this->registerCommand("/delete", [deleteMessages](const QStringList &words,
                                                      auto channel) {
        // This is a wrapper over the Helix delete messages endpoint
        // We use this to ensure the user gets better error messages for missing or malformed arguments
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /delete command only works in Twitch channels"));
            return "";
        }
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

        return deleteMessages(twitchChannel, messageID);
    });

    this->registerCommand("/mod", [](const QStringList &words, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /mod command only works in Twitch channels"));
            return "";
        }
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/mod <username>\" - Grant moderator status to a "
                "user. Use \"/mods\" to list the moderators of this channel."));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to mod someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [twitchChannel, channel](const HelixUser &targetUser) {
                getHelix()->addChannelModerator(
                    twitchChannel->roomId(), targetUser.id,
                    [channel, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString("You have added %1 as a moderator of this "
                                    "channel.")
                                .arg(targetUser.displayName)));
                    },
                    [channel, targetUser](auto error, auto message) {
                        QString errorMessage =
                            QString("Failed to add channel moderator - ");

                        using Error = HelixAddChannelModeratorError;

                        switch (error)
                        {
                            case Error::UserMissingScope: {
                                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                errorMessage += "Missing required scope. "
                                                "Re-login with your "
                                                "account and try again.";
                            }
                            break;

                            case Error::UserNotAuthorized: {
                                // TODO(pajlada): Phrase MISSING_PERMISSION
                                errorMessage += "You don't have permission to "
                                                "perform that action.";
                            }
                            break;

                            case Error::Ratelimited: {
                                errorMessage +=
                                    "You are being ratelimited by Twitch. Try "
                                    "again in a few seconds.";
                            }
                            break;

                            case Error::TargetIsVIP: {
                                errorMessage +=
                                    QString("%1 is currently a VIP, \"/unvip\" "
                                            "them and "
                                            "retry this command.")
                                        .arg(targetUser.displayName);
                            }
                            break;

                            case Error::TargetAlreadyModded: {
                                // Equivalent irc error
                                errorMessage =
                                    QString("%1 is already a moderator of this "
                                            "channel.")
                                        .arg(targetUser.displayName);
                            }
                            break;

                            case Error::Forwarded: {
                                errorMessage += message;
                            }
                            break;

                            case Error::Unknown:
                            default: {
                                errorMessage +=
                                    "An unknown error has occurred.";
                            }
                            break;
                        }
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    });

    this->registerCommand("/unmod", [](const QStringList &words, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /unmod command only works in Twitch channels"));
            return "";
        }
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/unmod <username>\" - Revoke moderator status from a "
                "user. Use \"/mods\" to list the moderators of this channel."));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to unmod someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [twitchChannel, channel](const HelixUser &targetUser) {
                getHelix()->removeChannelModerator(
                    twitchChannel->roomId(), targetUser.id,
                    [channel, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString("You have removed %1 as a moderator of "
                                    "this channel.")
                                .arg(targetUser.displayName)));
                    },
                    [channel, targetUser](auto error, auto message) {
                        QString errorMessage =
                            QString("Failed to remove channel moderator - ");

                        using Error = HelixRemoveChannelModeratorError;

                        switch (error)
                        {
                            case Error::UserMissingScope: {
                                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                errorMessage += "Missing required scope. "
                                                "Re-login with your "
                                                "account and try again.";
                            }
                            break;

                            case Error::UserNotAuthorized: {
                                // TODO(pajlada): Phrase MISSING_PERMISSION
                                errorMessage += "You don't have permission to "
                                                "perform that action.";
                            }
                            break;

                            case Error::Ratelimited: {
                                errorMessage +=
                                    "You are being ratelimited by Twitch. Try "
                                    "again in a few seconds.";
                            }
                            break;

                            case Error::TargetNotModded: {
                                // Equivalent irc error
                                errorMessage +=
                                    QString("%1 is not a moderator of this "
                                            "channel.")
                                        .arg(targetUser.displayName);
                            }
                            break;

                            case Error::Forwarded: {
                                errorMessage += message;
                            }
                            break;

                            case Error::Unknown:
                            default: {
                                errorMessage +=
                                    "An unknown error has occurred.";
                            }
                            break;
                        }
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    });

    this->registerCommand(
        "/announce", [](const QStringList &words, auto channel) -> QString {
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "This command can only be used in Twitch channels."));
                return "";
            }

            if (words.size() < 2)
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: /announce <message> - Call attention to your "
                    "message with a highlight."));
                return "";
            }

            auto user = getApp()->accounts->twitch.getCurrent();
            if (user->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to use the /announce command"));
                return "";
            }

            getHelix()->sendChatAnnouncement(
                twitchChannel->roomId(), user->getUserId(),
                words.mid(1).join(" "), HelixAnnouncementColor::Primary,
                []() {
                    // do nothing.
                },
                [channel](auto error, auto message) {
                    using Error = HelixSendChatAnnouncementError;
                    QString errorMessage =
                        QString("Failed to send announcement - ");

                    switch (error)
                    {
                        case Error::UserMissingScope: {
                            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                            errorMessage +=
                                "Missing required scope. Re-login with your "
                                "account and try again.";
                        }
                        break;

                        case Error::Forwarded: {
                            errorMessage += message;
                        }
                        break;

                        case Error::Unknown:
                        default: {
                            errorMessage += "An unknown error has occurred.";
                        }
                        break;
                    }

                    channel->addMessage(makeSystemMessage(errorMessage));
                });
            return "";
        });

    this->registerCommand("/vip", [](const QStringList &words, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /vip command only works in Twitch channels"));
            return "";
        }
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/vip <username>\" - Grant VIP status to a user. Use "
                "\"/vips\" to list the VIPs of this channel."));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to VIP someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [twitchChannel, channel](const HelixUser &targetUser) {
                getHelix()->addChannelVIP(
                    twitchChannel->roomId(), targetUser.id,
                    [channel, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString(
                                "You have added %1 as a VIP of this channel.")
                                .arg(targetUser.displayName)));
                    },
                    [channel, targetUser](auto error, auto message) {
                        QString errorMessage = QString("Failed to add VIP - ");

                        using Error = HelixAddChannelVIPError;

                        switch (error)
                        {
                            case Error::UserMissingScope: {
                                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                errorMessage += "Missing required scope. "
                                                "Re-login with your "
                                                "account and try again.";
                            }
                            break;

                            case Error::UserNotAuthorized: {
                                // TODO(pajlada): Phrase MISSING_PERMISSION
                                errorMessage += "You don't have permission to "
                                                "perform that action.";
                            }
                            break;

                            case Error::Ratelimited: {
                                errorMessage +=
                                    "You are being ratelimited by Twitch. Try "
                                    "again in a few seconds.";
                            }
                            break;

                            case Error::Forwarded: {
                                // These are actually the IRC equivalents, so we can ditch the prefix
                                errorMessage = message;
                            }
                            break;

                            case Error::Unknown:
                            default: {
                                errorMessage +=
                                    "An unknown error has occurred.";
                            }
                            break;
                        }
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    });

    this->registerCommand("/unvip", [](const QStringList &words, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                "The /unvip command only works in Twitch channels"));
            return "";
        }
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                "Usage: \"/unvip <username>\" - Revoke VIP status from a user. "
                "Use \"/vips\" to list the VIPs of this channel."));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to UnVIP someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [twitchChannel, channel](const HelixUser &targetUser) {
                getHelix()->removeChannelVIP(
                    twitchChannel->roomId(), targetUser.id,
                    [channel, targetUser] {
                        channel->addMessage(makeSystemMessage(
                            QString(
                                "You have removed %1 as a VIP of this channel.")
                                .arg(targetUser.displayName)));
                    },
                    [channel, targetUser](auto error, auto message) {
                        QString errorMessage =
                            QString("Failed to remove VIP - ");

                        using Error = HelixRemoveChannelVIPError;

                        switch (error)
                        {
                            case Error::UserMissingScope: {
                                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                errorMessage += "Missing required scope. "
                                                "Re-login with your "
                                                "account and try again.";
                            }
                            break;

                            case Error::UserNotAuthorized: {
                                // TODO(pajlada): Phrase MISSING_PERMISSION
                                errorMessage += "You don't have permission to "
                                                "perform that action.";
                            }
                            break;

                            case Error::Ratelimited: {
                                errorMessage +=
                                    "You are being ratelimited by Twitch. Try "
                                    "again in a few seconds.";
                            }
                            break;

                            case Error::Forwarded: {
                                // These are actually the IRC equivalents, so we can ditch the prefix
                                errorMessage = message;
                            }
                            break;

                            case Error::Unknown:
                            default: {
                                errorMessage +=
                                    "An unknown error has occurred.";
                            }
                            break;
                        }
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    });

    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    auto unbanLambda = [](auto words, auto channel) {
        auto commandName = words.at(0).toLower();
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                QString("The %1 command only works in Twitch channels")
                    .arg(commandName)));
            return "";
        }
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(
                QString("Usage: \"%1 <username>\" - Removes a ban on a user.")
                    .arg(commandName)));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to unban someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        getHelix()->getUserByName(
            target,
            [channel, currentUser, twitchChannel,
             target](const auto &targetUser) {
                getHelix()->unbanUser(
                    twitchChannel->roomId(), currentUser->getUserId(),
                    targetUser.id,
                    [] {
                        // No response for unbans, they're emitted over pubsub/IRC instead
                    },
                    [channel, target, targetUser](auto error, auto message) {
                        using Error = HelixUnbanUserError;

                        QString errorMessage =
                            QString("Failed to unban user - ");

                        switch (error)
                        {
                            case Error::ConflictingOperation: {
                                errorMessage +=
                                    "There was a conflicting ban operation on "
                                    "this user. Please try again.";
                            }
                            break;

                            case Error::Forwarded: {
                                errorMessage += message;
                            }
                            break;

                            case Error::Ratelimited: {
                                errorMessage +=
                                    "You are being ratelimited by Twitch. Try "
                                    "again in a few seconds.";
                            }
                            break;

                            case Error::TargetNotBanned: {
                                // Equivalent IRC error
                                errorMessage =
                                    QString(
                                        "%1 is not banned from this channel.")
                                        .arg(targetUser.displayName);
                            }
                            break;

                            case Error::UserMissingScope: {
                                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                errorMessage += "Missing required scope. "
                                                "Re-login with your "
                                                "account and try again.";
                            }
                            break;

                            case Error::UserNotAuthorized: {
                                // TODO(pajlada): Phrase MISSING_PERMISSION
                                errorMessage += "You don't have permission to "
                                                "perform that action.";
                            }
                            break;

                            case Error::Unknown: {
                                errorMessage +=
                                    "An unknown error has occurred.";
                            }
                            break;
                        }

                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    };  // These changes are from the helix-command-migration/unban-untimeout branch

    this->registerCommand("/unban", [unbanLambda](const QStringList &words,
                                                  auto channel) {
        return unbanLambda(words, channel);
    });  // These changes are from the helix-command-migration/unban-untimeout branch

    this->registerCommand("/untimeout", [unbanLambda](const QStringList &words,
                                                      auto channel) {
        return unbanLambda(words, channel);
    });  // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch
    // These changes are from the helix-command-migration/unban-untimeout branch

    this->registerCommand(  // /raid
        "/raid", [](const QStringList &words, auto channel) -> QString {
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /raid command only works in Twitch channels"));
                return "";
            }
            switch (getSettings()->helixTimegateRaid.getValue())
            {
                case HelixTimegateOverride::Timegate: {
                    if (areIRCCommandsStillAvailable())
                    {
                        return useIRCCommand(words);
                    }

                    // fall through to Helix logic
                }
                break;

                case HelixTimegateOverride::AlwaysUseIRC: {
                    return useIRCCommand(words);
                }
                break;

                case HelixTimegateOverride::AlwaysUseHelix: {
                    // do nothing and fall through to Helix logic
                }
                break;
            }

            if (words.size() < 2)
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: \"/raid <username>\" - Raid a user. "
                    "Only the broadcaster can start a raid."));
                return "";
            }

            auto currentUser = getApp()->accounts->twitch.getCurrent();
            if (currentUser->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to start a raid!"));
                return "";
            }

            auto target = words.at(1);
            stripChannelName(target);

            getHelix()->getUserByName(
                target,
                [twitchChannel, channel](const HelixUser &targetUser) {
                    getHelix()->startRaid(
                        twitchChannel->roomId(), targetUser.id,
                        [channel, targetUser] {
                            channel->addMessage(makeSystemMessage(
                                QString("You started to raid %1.")
                                    .arg(targetUser.displayName)));
                        },
                        [channel, targetUser](auto error, auto message) {
                            QString errorMessage =
                                QString("Failed to start a raid - ");

                            using Error = HelixStartRaidError;

                            switch (error)
                            {
                                case Error::UserMissingScope: {
                                    // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                                    errorMessage += "Missing required scope. "
                                                    "Re-login with your "
                                                    "account and try again.";
                                }
                                break;

                                case Error::UserNotAuthorized: {
                                    errorMessage +=
                                        "You must be the broadcaster "
                                        "to start a raid.";
                                }
                                break;

                                case Error::CantRaidYourself: {
                                    errorMessage +=
                                        "A channel cannot raid itself.";
                                }
                                break;

                                case Error::Ratelimited: {
                                    errorMessage += "You are being ratelimited "
                                                    "by Twitch. Try "
                                                    "again in a few seconds.";
                                }
                                break;

                                case Error::Forwarded: {
                                    errorMessage += message;
                                }
                                break;

                                case Error::Unknown:
                                default: {
                                    errorMessage +=
                                        "An unknown error has occurred.";
                                }
                                break;
                            }
                            channel->addMessage(
                                makeSystemMessage(errorMessage));
                        });
                },
                [channel, target] {
                    // Equivalent error from IRC
                    channel->addMessage(makeSystemMessage(
                        QString("Invalid username: %1").arg(target)));
                });

            return "";
        });  // /raid

    this->registerCommand(  // /unraid
        "/unraid", [](const QStringList &words, auto channel) -> QString {
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /unraid command only works in Twitch channels"));
                return "";
            }
            switch (getSettings()->helixTimegateRaid.getValue())
            {
                case HelixTimegateOverride::Timegate: {
                    if (areIRCCommandsStillAvailable())
                    {
                        return useIRCCommand(words);
                    }

                    // fall through to Helix logic
                }
                break;

                case HelixTimegateOverride::AlwaysUseIRC: {
                    return useIRCCommand(words);
                }
                break;

                case HelixTimegateOverride::AlwaysUseHelix: {
                    // do nothing and fall through to Helix logic
                }
                break;
            }

            if (words.size() != 1)
            {
                channel->addMessage(makeSystemMessage(
                    "Usage: \"/unraid\" - Cancel the current raid. "
                    "Only the broadcaster can cancel the raid."));
                return "";
            }

            auto currentUser = getApp()->accounts->twitch.getCurrent();
            if (currentUser->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to cancel the raid!"));
                return "";
            }

            getHelix()->cancelRaid(
                twitchChannel->roomId(),
                [channel] {
                    channel->addMessage(
                        makeSystemMessage(QString("You cancelled the raid.")));
                },
                [channel](auto error, auto message) {
                    QString errorMessage =
                        QString("Failed to cancel the raid - ");

                    using Error = HelixCancelRaidError;

                    switch (error)
                    {
                        case Error::UserMissingScope: {
                            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                            errorMessage += "Missing required scope. "
                                            "Re-login with your "
                                            "account and try again.";
                        }
                        break;

                        case Error::UserNotAuthorized: {
                            errorMessage += "You must be the broadcaster "
                                            "to cancel the raid.";
                        }
                        break;

                        case Error::NoRaidPending: {
                            errorMessage += "You don't have an active raid.";
                        }
                        break;

                        case Error::Ratelimited: {
                            errorMessage +=
                                "You are being ratelimited by Twitch. Try "
                                "again in a few seconds.";
                        }
                        break;

                        case Error::Forwarded: {
                            errorMessage += message;
                        }
                        break;

                        case Error::Unknown:
                        default: {
                            errorMessage += "An unknown error has occurred.";
                        }
                        break;
                    }
                    channel->addMessage(makeSystemMessage(errorMessage));
                });

            return "";
        });  // unraid

    this->registerCommand("/emoteonly", &commands::emoteOnly);
    this->registerCommand("/emoteonlyoff", &commands::emoteOnlyOff);

    this->registerCommand("/subscribers", &commands::subscribers);
    this->registerCommand("/subscribersoff", &commands::subscribersOff);

    this->registerCommand("/slow", &commands::slow);
    this->registerCommand("/slowoff", &commands::slowOff);

    this->registerCommand("/followers", &commands::followers);
    this->registerCommand("/followersoff", &commands::followersOff);

    this->registerCommand("/uniquechat", &commands::uniqueChat);
    this->registerCommand("/r9kbeta", &commands::uniqueChat);
    this->registerCommand("/uniquechatoff", &commands::uniqueChatOff);
    this->registerCommand("/r9kbetaoff", &commands::uniqueChatOff);

    auto formatBanTimeoutError =
        [](const char *operation, HelixBanUserError error,
           const QString &message, const QString &userDisplayName) -> QString {
        using Error = HelixBanUserError;

        QString errorMessage = QString("Failed to %1 user - ").arg(operation);

        switch (error)
        {
            case Error::ConflictingOperation: {
                errorMessage += "There was a conflicting ban operation on "
                                "this user. Please try again.";
            }
            break;

            case Error::Forwarded: {
                errorMessage += message;
            }
            break;

            case Error::Ratelimited: {
                errorMessage += "You are being ratelimited by Twitch. Try "
                                "again in a few seconds.";
            }
            break;

            case Error::TargetBanned: {
                // Equivalent IRC error
                errorMessage += QString("%1 is already banned in this channel.")
                                    .arg(userDisplayName);
            }
            break;

            case Error::CannotBanUser: {
                // We can't provide the identical error as in IRC,
                // because we don't have enough information about the user.
                // The messages from IRC are formatted like this:
                // "You cannot {op} moderator {mod} unless you are the owner of this channel."
                // "You cannot {op} the broadcaster."
                errorMessage += QString("You cannot %1 %2.")
                                    .arg(operation, userDisplayName);
            }
            break;

            case Error::UserMissingScope: {
                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                errorMessage += "Missing required scope. "
                                "Re-login with your "
                                "account and try again.";
            }
            break;

            case Error::UserNotAuthorized: {
                // TODO(pajlada): Phrase MISSING_PERMISSION
                errorMessage += "You don't have permission to "
                                "perform that action.";
            }
            break;

            case Error::Unknown: {
                errorMessage += "An unknown error has occurred.";
            }
            break;
        }
        return errorMessage;
    };

    this->registerCommand("/timeout", [formatBanTimeoutError](
                                          const QStringList &words,
                                          auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                QString("The /timeout command only works in Twitch channels")));
            return "";
        }
        const auto *usageStr =
            "Usage: \"/timeout <username> [duration][time unit] [reason]\" - "
            "Temporarily prevent a user from chatting. Duration (optional, "
            "default=10 minutes) must be a positive integer; time unit "
            "(optional, default=s) must be one of s, m, h, d, w; maximum "
            "duration is 2 weeks. Combinations like 1d2h are also allowed. "
            "Reason is optional and will be shown to the target user and other "
            "moderators. Use \"/untimeout\" to remove a timeout.";
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(usageStr));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to timeout someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        int duration = 10 * 60;  // 10min
        if (words.size() >= 3)
        {
            duration = (int)parseDurationToSeconds(words.at(2));
            if (duration <= 0)
            {
                channel->addMessage(makeSystemMessage(usageStr));
                return "";
            }
        }
        auto reason = words.mid(3).join(' ');

        getHelix()->getUserByName(
            target,
            [channel, currentUser, twitchChannel, target, duration, reason,
             formatBanTimeoutError](const auto &targetUser) {
                getHelix()->banUser(
                    twitchChannel->roomId(), currentUser->getUserId(),
                    targetUser.id, duration, reason,
                    [] {
                        // No response for timeouts, they're emitted over pubsub/IRC instead
                    },
                    [channel, target, targetUser, formatBanTimeoutError](
                        auto error, auto message) {
                        auto errorMessage = formatBanTimeoutError(
                            "timeout", error, message, targetUser.displayName);
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    });

    this->registerCommand("/ban", [formatBanTimeoutError](
                                      const QStringList &words, auto channel) {
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
        if (twitchChannel == nullptr)
        {
            channel->addMessage(makeSystemMessage(
                QString("The /ban command only works in Twitch channels")));
            return "";
        }

        const auto *usageStr =
            "Usage: \"/ban <username> [reason]\" - Permanently prevent a user "
            "from chatting. Reason is optional and will be shown to the target "
            "user and other moderators. Use \"/unban\" to remove a ban.";
        if (words.size() < 2)
        {
            channel->addMessage(makeSystemMessage(usageStr));
            return "";
        }

        auto currentUser = getApp()->accounts->twitch.getCurrent();
        if (currentUser->isAnon())
        {
            channel->addMessage(
                makeSystemMessage("You must be logged in to ban someone!"));
            return "";
        }

        auto target = words.at(1);
        stripChannelName(target);

        auto reason = words.mid(2).join(' ');

        getHelix()->getUserByName(
            target,
            [channel, currentUser, twitchChannel, target, reason,
             formatBanTimeoutError](const auto &targetUser) {
                getHelix()->banUser(
                    twitchChannel->roomId(), currentUser->getUserId(),
                    targetUser.id, boost::none, reason,
                    [] {
                        // No response for bans, they're emitted over pubsub/IRC instead
                    },
                    [channel, target, targetUser, formatBanTimeoutError](
                        auto error, auto message) {
                        auto errorMessage = formatBanTimeoutError(
                            "ban", error, message, targetUser.displayName);
                        channel->addMessage(makeSystemMessage(errorMessage));
                    });
            },
            [channel, target] {
                // Equivalent error from IRC
                channel->addMessage(makeSystemMessage(
                    QString("Invalid username: %1").arg(target)));
            });

        return "";
    });

    for (const auto &cmd : TWITCH_WHISPER_COMMANDS)
    {
        this->registerCommand(cmd, [](const QStringList &words, auto channel) {
            return runWhisperCommand(words, channel);
        });
    }

    auto formatVIPListError = [](HelixListVIPsError error,
                                 const QString &message) -> QString {
        using Error = HelixListVIPsError;

        QString errorMessage = QString("Failed to list VIPs - ");

        switch (error)
        {
            case Error::Forwarded: {
                errorMessage += message;
            }
            break;

            case Error::Ratelimited: {
                errorMessage += "You are being ratelimited by Twitch. Try "
                                "again in a few seconds.";
            }
            break;

            case Error::UserMissingScope: {
                // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
                errorMessage += "Missing required scope. "
                                "Re-login with your "
                                "account and try again.";
            }
            break;

            case Error::UserNotAuthorized: {
                // TODO(pajlada): Phrase MISSING_PERMISSION
                errorMessage += "You don't have permission to "
                                "perform that action.";
            }
            break;

            case Error::UserNotBroadcaster: {
                errorMessage +=
                    "Due to Twitch restrictions, "
                    "this command can only be used by the broadcaster. "
                    "To see the list of VIPs you must use the Twitch website.";
            }
            break;

            case Error::Unknown: {
                errorMessage += "An unknown error has occurred.";
            }
            break;
        }
        return errorMessage;
    };

    auto formatStartCommercialError = [](HelixStartCommercialError error,
                                         const QString &message) -> QString {
        using Error = HelixStartCommercialError;

        QString errorMessage = "Failed to start commercial - ";

        switch (error)
        {
            case Error::UserMissingScope: {
                errorMessage += "Missing required scope. Re-login with your "
                                "account and try again.";
            }
            break;

            case Error::TokenMustMatchBroadcaster: {
                errorMessage += "Only the broadcaster of the channel can run "
                                "commercials.";
            }
            break;

            case Error::BroadcasterNotStreaming: {
                errorMessage += "You must be streaming live to run "
                                "commercials.";
            }
            break;

            case Error::MissingLengthParameter: {
                errorMessage +=
                    "Command must include a desired commercial break "
                    "length that is greater than zero.";
            }
            break;

            case Error::Ratelimited: {
                errorMessage += "You must wait until your cooldown period "
                                "expires before you can run another "
                                "commercial.";
            }
            break;

            case Error::Forwarded: {
                errorMessage += message;
            }
            break;

            case Error::Unknown:
            default: {
                errorMessage +=
                    QString("An unknown error has occurred (%1).").arg(message);
            }
            break;
        }

        return errorMessage;
    };

    this->registerCommand(
        "/vips",
        [formatVIPListError](const QStringList &words,
                             auto channel) -> QString {
            auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
            if (twitchChannel == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /vips command only works in Twitch channels"));
                return "";
            }

            switch (getSettings()->helixTimegateVIPs.getValue())
            {
                case HelixTimegateOverride::Timegate: {
                    if (areIRCCommandsStillAvailable())
                    {
                        return useIRCCommand(words);
                    }

                    // fall through to Helix logic
                }
                break;

                case HelixTimegateOverride::AlwaysUseIRC: {
                    return useIRCCommand(words);
                }
                break;

                case HelixTimegateOverride::AlwaysUseHelix: {
                    // do nothing and fall through to Helix logic
                }
                break;
            }
            auto currentUser = getApp()->accounts->twitch.getCurrent();
            if (currentUser->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "Due to Twitch restrictions, "  //
                    "this command can only be used by the broadcaster. "
                    "To see the list of VIPs you must use the "
                    "Twitch website."));
                return "";
            }

            getHelix()->getChannelVIPs(
                twitchChannel->roomId(),
                [channel, twitchChannel](const std::vector<HelixVip> &vipList) {
                    if (vipList.empty())
                    {
                        channel->addMessage(makeSystemMessage(
                            "This channel does not have any VIPs."));
                        return;
                    }

                    auto messagePrefix =
                        QString("The VIPs of this channel are");

                    // TODO: sort results?
                    MessageBuilder builder;
                    TwitchMessageBuilder::listOfUsersSystemMessage(
                        messagePrefix, vipList, twitchChannel, &builder);

                    channel->addMessage(builder.release());
                },
                [channel, formatVIPListError](auto error, auto message) {
                    auto errorMessage = formatVIPListError(error, message);
                    channel->addMessage(makeSystemMessage(errorMessage));
                });

            return "";
        });

    this->registerCommand(
        "/commercial",
        [formatStartCommercialError](const QStringList &words,
                                     auto channel) -> QString {
            auto *tc = dynamic_cast<TwitchChannel *>(channel.get());
            if (tc == nullptr)
            {
                channel->addMessage(makeSystemMessage(
                    "The /commercial command only works in Twitch channels"));
                return "";
            }

            const auto *usageStr = "Usage: \"/commercial <length>\" - Starts a "
                                   "commercial with the "
                                   "specified duration for the current "
                                   "channel. Valid length options "
                                   "are 30, 60, 90, 120, 150, and 180 seconds.";

            switch (getSettings()->helixTimegateCommercial.getValue())
            {
                case HelixTimegateOverride::Timegate: {
                    if (areIRCCommandsStillAvailable())
                    {
                        return useIRCCommand(words);
                    }

                    // fall through to Helix logic
                }
                break;

                case HelixTimegateOverride::AlwaysUseIRC: {
                    return useIRCCommand(words);
                }
                break;

                case HelixTimegateOverride::AlwaysUseHelix: {
                    // do nothing and fall through to Helix logic
                }
                break;
            }

            if (words.size() < 2)
            {
                channel->addMessage(makeSystemMessage(usageStr));
                return "";
            }

            auto user = getApp()->accounts->twitch.getCurrent();

            // Avoid Helix calls without Client ID and/or OAuth Token
            if (user->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to use the /commercial command"));
                return "";
            }

            auto broadcasterID = tc->roomId();
            auto length = words.at(1).toInt();

            getHelix()->startCommercial(
                broadcasterID, length,
                [channel](auto response) {
                    channel->addMessage(makeSystemMessage(
                        QString("Starting %1 second long commercial break. "
                                "Keep in mind you are still "
                                "live and not all viewers will receive a "
                                "commercial. "
                                "You may run another commercial in %2 seconds.")
                            .arg(response.length)
                            .arg(response.retryAfter)));
                },
                [channel, formatStartCommercialError](auto error,
                                                      auto message) {
                    auto errorMessage =
                        formatStartCommercialError(error, message);
                    channel->addMessage(makeSystemMessage(errorMessage));
                });

            return "";
        });

    this->registerCommand("/unstable-set-user-color", [](const auto &ctx) {
        if (ctx.twitchChannel == nullptr)
        {
            ctx.channel->addMessage(
                makeSystemMessage("The /unstable-set-user-color command only "
                                  "works in Twitch channels"));
            return "";
        }
        auto userID = ctx.words.at(1);
        if (ctx.words.size() < 2)
        {
            ctx.channel->addMessage(
                makeSystemMessage(QString("Usage: %1 <TwitchUserID> [color]")
                                      .arg(ctx.words.at(0))));
        }

        auto color = ctx.words.value(2);

        getIApp()->getUserData()->setUserColor(userID, color);

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

    if (!dryRun)
    {
        // check if command exists
        const auto it = this->commands_.find(commandName);
        if (it != this->commands_.end())
        {
            if (auto *command = std::get_if<CommandFunction>(&it->second))
            {
                return (*command)(words, channel);
            }
            if (auto *command =
                    std::get_if<CommandFunctionWithContext>(&it->second))
            {
                CommandContext ctx{
                    words,
                    channel,
                    dynamic_cast<TwitchChannel *>(channel.get()),
                };
                return (*command)(ctx);
            }

            return "";
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

void CommandController::registerCommand(const QString &commandName,
                                        CommandFunctionVariants commandFunction)
{
    assert(this->commands_.count(commandName) == 0);

    this->commands_[commandName] = std::move(commandFunction);

    this->defaultChatterinoCommandAutoCompletions_.append(commandName);
}

QString CommandController::execCustomCommand(
    const QStringList &words, const Command &command, bool /* dryRun */,
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

    return result.replace("{{", "{");
}

QStringList CommandController::getDefaultChatterinoCommandList()
{
    return this->defaultChatterinoCommandAutoCompletions_;
}

}  // namespace chatterino
