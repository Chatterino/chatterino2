#include "controllers/commands/CommandController.hpp"

#include "Application.hpp"
#include "common/LinkParser.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "common/SignalVector.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/builtin/chatterino/Debugging.hpp"
#include "controllers/commands/builtin/Misc.hpp"
#include "controllers/commands/builtin/twitch/AddModerator.hpp"
#include "controllers/commands/builtin/twitch/AddVIP.hpp"
#include "controllers/commands/builtin/twitch/Announce.hpp"
#include "controllers/commands/builtin/twitch/Ban.hpp"
#include "controllers/commands/builtin/twitch/Block.hpp"
#include "controllers/commands/builtin/twitch/ChatSettings.hpp"
#include "controllers/commands/builtin/twitch/Chatters.hpp"
#include "controllers/commands/builtin/twitch/DeleteMessages.hpp"
#include "controllers/commands/builtin/twitch/GetModerators.hpp"
#include "controllers/commands/builtin/twitch/GetVIPs.hpp"
#include "controllers/commands/builtin/twitch/Raid.hpp"
#include "controllers/commands/builtin/twitch/RemoveModerator.hpp"
#include "controllers/commands/builtin/twitch/RemoveVIP.hpp"
#include "controllers/commands/builtin/twitch/ShieldMode.hpp"
#include "controllers/commands/builtin/twitch/Shoutout.hpp"
#include "controllers/commands/builtin/twitch/StartCommercial.hpp"
#include "controllers/commands/builtin/twitch/Unban.hpp"
#include "controllers/commands/builtin/twitch/UpdateChannel.hpp"
#include "controllers/commands/builtin/twitch/UpdateColor.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "controllers/plugins/PluginController.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "messages/Image.hpp"
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
#include "util/PostToThread.hpp"
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
    auto emote = std::optional<EmotePtr>{};
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
                b.emplace<EmoteElement>(*emote, flags);
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
                        LinkParser parser(string);
                        if (parser.result())
                        {
                            b.addLink(*parser.result());
                        }
                        else
                        {
                            b.emplace<TextElement>(string,
                                                   MessageElementFlag::Text);
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

    auto overrideFlags = std::optional<MessageFlags>(messagexD->flags);
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
    // We can safely ignore these signal connections since items will be destroyed
    // before CommandController
    std::ignore = this->items.itemInserted.connect(addFirstMatchToMap);
    std::ignore = this->items.itemRemoved.connect(addFirstMatchToMap);

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
    std::ignore = this->items.delayedItemsChanged.connect([this] {
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

    this->registerCommand("/ignore", &commands::ignoreUser);

    this->registerCommand("/unignore", &commands::unignoreUser);

    this->registerCommand("/follow", &commands::follow);

    this->registerCommand("/unfollow", &commands::unfollow);

    /// Supported commands

    this->registerCommand("/debug-args", &commands::listArgs);

    this->registerCommand("/debug-env", &commands::listEnvironmentVariables);

    this->registerCommand("/uptime", &commands::uptime);

    this->registerCommand("/block", &commands::blockUser);

    this->registerCommand("/unblock", &commands::unblockUser);

    this->registerCommand("/user", &commands::user);

    this->registerCommand("/usercard", [](const auto &words, auto channel) {
        if (words.size() < 2)
        {
            channel->addMessage(
                makeSystemMessage("Usage: /usercard <username> [channel] or "
                                  "/usercard id:<id> [channel]"));
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
        userPopup->moveTo(QCursor::pos(),
                          widgets::BoundsChecking::CursorPosition);
        userPopup->show();
        return "";
    });

    this->registerCommand("/requests", &commands::requests);

    this->registerCommand("/lowtrust", &commands::lowtrust);

    this->registerCommand("/chatters", &commands::chatters);

    this->registerCommand("/test-chatters", &commands::testChatters);

    this->registerCommand("/mods", &commands::getModerators);

    this->registerCommand("/clip", &commands::clip);

    this->registerCommand("/marker", &commands::marker);

    this->registerCommand("/streamlink", &commands::streamlink);

    this->registerCommand("/popout", &commands::popout);

    this->registerCommand("/popup", &commands::popup);

    this->registerCommand("/clearmessages", &commands::clearmessages);

    this->registerCommand("/settitle", &commands::setTitle);

    this->registerCommand("/setgame", &commands::setGame);

    this->registerCommand("/openurl", &commands::openURL);

    this->registerCommand("/raw", &commands::sendRawMessage);

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
                    // found most recent message by user
                    if (msg->replyThread == nullptr)
                    {
                        // prepare thread if one does not exist
                        auto thread = std::make_shared<MessageThread>(msg);
                        twitchChannel->addReplyThread(thread);
                    }

                    QString reply = words.mid(2).join(" ");
                    twitchChannel->sendReply(reply, msg->id);
                    return "";
                }
            }

            channel->addMessage(
                makeSystemMessage("A message from that user wasn't found"));

            return "";
        });

#ifndef NDEBUG
    this->registerCommand("/fakemsg", &commands::injectFakeMessage);
#endif

    this->registerCommand("/copy", &commands::copyToClipboard);

    this->registerCommand("/color", &commands::updateUserColor);

    this->registerCommand("/clear", &commands::deleteAllMessages);

    this->registerCommand("/delete", &commands::deleteOneMessage);

    this->registerCommand("/mod", &commands::addModerator);

    this->registerCommand("/unmod", &commands::removeModerator);

    this->registerCommand("/announce", &commands::sendAnnouncement);

    this->registerCommand("/vip", &commands::addVIP);

    this->registerCommand("/unvip", &commands::removeVIP);

    this->registerCommand("/unban", &commands::unbanUser);
    this->registerCommand("/untimeout", &commands::unbanUser);

    this->registerCommand("/raid", &commands::startRaid);

    this->registerCommand("/unraid", &commands::cancelRaid);

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

    this->registerCommand("/timeout", &commands::sendTimeout);

    this->registerCommand("/ban", &commands::sendBan);
    this->registerCommand("/banid", &commands::sendBanById);

    for (const auto &cmd : TWITCH_WHISPER_COMMANDS)
    {
        this->registerCommand(cmd, [](const QStringList &words, auto channel) {
            return runWhisperCommand(words, channel);
        });
    }

    this->registerCommand("/vips", &commands::getVIPs);

    this->registerCommand("/commercial", &commands::startCommercial);

    this->registerCommand("/unstable-set-user-color", [](const auto &ctx) {
        if (ctx.twitchChannel == nullptr)
        {
            ctx.channel->addMessage(
                makeSystemMessage("The /unstable-set-user-color command only "
                                  "works in Twitch channels"));
            return "";
        }
        if (ctx.words.size() < 2)
        {
            ctx.channel->addMessage(
                makeSystemMessage(QString("Usage: %1 <TwitchUserID> [color]")
                                      .arg(ctx.words.at(0))));
            return "";
        }

        auto userID = ctx.words.at(1);

        auto color = ctx.words.value(2);

        getIApp()->getUserData()->setUserColor(userID, color);

        return "";
    });

    this->registerCommand(
        "/debug-force-image-gc",
        [](const QStringList & /*words*/, auto /*channel*/) -> QString {
            runInGuiThread([] {
                using namespace chatterino::detail;
                auto &iep = ImageExpirationPool::instance();
                iep.freeOld();
            });
            return "";
        });

    this->registerCommand(
        "/debug-force-image-unload",
        [](const QStringList & /*words*/, auto /*channel*/) -> QString {
            runInGuiThread([] {
                using namespace chatterino::detail;
                auto &iep = ImageExpirationPool::instance();
                iep.freeAll();
            });
            return "";
        });

    this->registerCommand("/shield", &commands::shieldModeOn);
    this->registerCommand("/shieldoff", &commands::shieldModeOff);

    this->registerCommand("/shoutout", &commands::sendShoutout);

    this->registerCommand("/c2-set-logging-rules", &commands::setLoggingRules);
    this->registerCommand("/c2-theme-autoreload", &commands::toggleThemeReload);
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

    // We have checks to ensure words cannot be empty, so this can never wrap around
    auto maxSpaces = std::min(this->maxSpaces_, (qsizetype)words.length() - 1);
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

#ifdef CHATTERINO_HAVE_PLUGINS
bool CommandController::registerPluginCommand(const QString &commandName)
{
    if (this->commands_.contains(commandName))
    {
        return false;
    }

    this->commands_[commandName] = [commandName](const CommandContext &ctx) {
        return getApp()->plugins->tryExecPluginCommand(commandName, ctx);
    };
    this->pluginCommands_.append(commandName);
    return true;
}

bool CommandController::unregisterPluginCommand(const QString &commandName)
{
    if (!this->pluginCommands_.contains(commandName))
    {
        return false;
    }
    this->pluginCommands_.removeAll(commandName);
    return this->commands_.erase(commandName) != 0;
}
#endif

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
