#include "CommandController.hpp"

#include "Application.hpp"
#include "common/SignalVector.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchApi.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/CombinePath.hpp"
#include "widgets/dialogs/LogsPopup.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"

#include <QApplication>
#include <QFile>
#include <QRegularExpression>

#define TWITCH_DEFAULT_COMMANDS                                         \
    {                                                                   \
        "/help", "/w", "/me", "/disconnect", "/mods", "/color", "/ban", \
            "/unban", "/timeout", "/untimeout", "/slow", "/slowoff",    \
            "/r9kbeta", "/r9kbetaoff", "/emoteonly", "/emoteonlyoff",   \
            "/clear", "/subscribers", "/subscribersoff", "/followers",  \
            "/followersoff", "/user"                                    \
    }

namespace {
using namespace chatterino;

static const QStringList whisperCommands{"/w", ".w"};

void sendWhisperMessage(const QString &text)
{
    // (hemirt) pajlada: "we should not be sending whispers through jtv, but
    // rather to your own username"
    auto app = getApp();
    app->twitch.server->sendMessage("jtv", text.simplified());
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
    const auto &bttvemotes = app->twitch.server->getBttvEmotes();
    const auto &ffzemotes = app->twitch.server->getFfzEmotes();
    auto flags = MessageElementFlags();
    auto emote = boost::optional<EmotePtr>{};
    for (int i = 2; i < words.length(); i++)
    {
        {  // twitch emote
            auto it = accemotes.emotes.find({words[i]});
            if (it != accemotes.emotes.end())
            {
                b.emplace<EmoteElement>(it->second,
                                        MessageElementFlag::TwitchEmote);
                continue;
            }
        }  // twitch emote

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
                boost::apply_visitor([&b](auto &&arg) { visitor(arg, b); },
                                     variant);
            }  // emoji/text
        }
    }

    b->flags.set(MessageFlag::DoNotTriggerNotification);
    b->flags.set(MessageFlag::Whisper);
    auto messagexD = b.release();

    app->twitch.server->whispersChannel->addMessage(messagexD);

    auto overrideFlags = boost::optional<MessageFlags>(messagexD->flags);
    overrideFlags->set(MessageFlag::DoNotLog);

    if (getSettings()->inlineWhispers)
    {
        app->twitch.server->forEachChannel(
            [&messagexD, overrideFlags](ChannelPtr _channel) {
                _channel->addMessage(messagexD, overrideFlags);
            });
    }

    return true;
}

bool appendWhisperMessageStringLocally(const QString &textNoEmoji)
{
    QString text = getApp()->emotes->emojis.replaceShortCodes(textNoEmoji);
    QStringList words = text.split(' ', QString::SkipEmptyParts);

    if (words.length() == 0)
    {
        return false;
    }

    QString commandName = words[0];

    if (whisperCommands.contains(commandName, Qt::CaseInsensitive))
    {
        if (words.length() > 2)
        {
            return appendWhisperMessageWordsLocally(words);
        }
    }
    return false;
}
}  // namespace

namespace chatterino {

void CommandController::initialize(Settings &, Paths &paths)
{
    // Update commands map when the vector of commands has been updated
    auto addFirstMatchToMap = [this](auto args) {
        this->commandsMap_.remove(args.item.name);

        for (const Command &cmd : this->items_)
        {
            if (cmd.name == args.item.name)
            {
                this->commandsMap_[cmd.name] = cmd;
                break;
            }
        }

        int maxSpaces = 0;

        for (const Command &cmd : this->items_)
        {
            auto localMaxSpaces = cmd.name.count(' ');
            if (localMaxSpaces > maxSpaces)
            {
                maxSpaces = localMaxSpaces;
            }
        }

        this->maxSpaces_ = maxSpaces;
    };
    this->items_.itemInserted.connect(addFirstMatchToMap);
    this->items_.itemRemoved.connect(addFirstMatchToMap);

    // Initialize setting manager for commands.json
    auto path = combinePath(paths.settingsDirectory, "commands.json");
    this->sm_ = std::make_shared<pajlada::Settings::SettingManager>();
    this->sm_->setPath(path.toStdString());

    // Delayed initialization of the setting storing all commands
    this->commandsSetting_.reset(
        new pajlada::Settings::Setting<std::vector<Command>>("/commands",
                                                             this->sm_));

    // Update the setting when the vector of commands has been updated (most
    // likely from the settings dialog)
    this->items_.delayedItemsChanged.connect([this] {  //
        this->commandsSetting_->setValue(this->items_.raw());
    });

    // Load commands from commands.json
    this->sm_->load();

    // Add loaded commands to our vector of commands (which will update the map
    // of commands)
    for (const auto &command : this->commandsSetting_->getValue())
    {
        this->items_.append(command);
    }
}

void CommandController::save()
{
    this->sm_->save();
}

CommandModel *CommandController::createModel(QObject *parent)
{
    CommandModel *model = new CommandModel(parent);
    model->initialize(&this->items_);

    return model;
}

QString CommandController::execCommand(const QString &textNoEmoji,
                                       ChannelPtr channel, bool dryRun)
{
    QString text = getApp()->emotes->emojis.replaceShortCodes(textNoEmoji);
    QStringList words = text.split(' ', QString::SkipEmptyParts);

    if (words.length() == 0)
    {
        return text;
    }

    QString commandName = words[0];

    // works in a valid twitch channel and /whispers, etc...
    if (!dryRun && channel->isTwitchChannel())
    {
        if (whisperCommands.contains(commandName, Qt::CaseInsensitive))
        {
            if (words.length() > 2)
            {
                appendWhisperMessageWordsLocally(words);
                sendWhisperMessage(text);
            }

            return "";
        }
    }

    // check if default command exists
    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    // works only in a valid twitch channel
    if (!dryRun && twitchChannel != nullptr)
    {
        if (commandName == "/debug-args")
        {
            QString msg = QApplication::instance()->arguments().join(' ');

            channel->addMessage(makeSystemMessage(msg));

            return "";
        }
        else if (commandName == "/uptime")
        {
            const auto &streamStatus = twitchChannel->accessStreamStatus();

            QString messageText = streamStatus->live ? streamStatus->uptime
                                                     : "Channel is not live.";

            channel->addMessage(makeSystemMessage(messageText));

            return "";
        }
        else if (commandName == "/ignore")
        {
            if (words.size() < 2)
            {
                channel->addMessage(makeSystemMessage("Usage: /ignore [user]"));
                return "";
            }
            auto app = getApp();

            auto user = app->accounts->twitch.getCurrent();
            auto target = words.at(1);

            if (user->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to ignore someone"));
                return "";
            }

            user->ignore(target,
                         [channel](auto resultCode, const QString &message) {
                             channel->addMessage(makeSystemMessage(message));
                         });

            return "";
        }
        else if (commandName == "/unignore")
        {
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /unignore [user]"));
                return "";
            }
            auto app = getApp();

            auto user = app->accounts->twitch.getCurrent();
            auto target = words.at(1);

            if (user->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to ignore someone"));
                return "";
            }

            user->unignore(target,
                           [channel](auto resultCode, const QString &message) {
                               channel->addMessage(makeSystemMessage(message));
                           });

            return "";
        }
        else if (commandName == "/follow")
        {
            if (words.size() < 2)
            {
                channel->addMessage(makeSystemMessage("Usage: /follow [user]"));
                return "";
            }
            auto app = getApp();

            auto user = app->accounts->twitch.getCurrent();
            auto target = words.at(1);

            if (user->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to follow someone"));
                return "";
            }

            TwitchApi::findUserId(
                target, [user, channel, target](QString userId) {
                    if (userId.isEmpty())
                    {
                        channel->addMessage(makeSystemMessage(
                            "User " + target + " could not be followed!"));
                        return;
                    }
                    user->followUser(userId, [channel, target]() {
                        channel->addMessage(makeSystemMessage(
                            "You successfully followed " + target));
                    });
                });

            return "";
        }
        else if (commandName == "/unfollow")
        {
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /unfollow [user]"));
                return "";
            }
            auto app = getApp();

            auto user = app->accounts->twitch.getCurrent();
            auto target = words.at(1);

            if (user->isAnon())
            {
                channel->addMessage(makeSystemMessage(
                    "You must be logged in to follow someone"));
                return "";
            }

            TwitchApi::findUserId(
                target, [user, channel, target](QString userId) {
                    if (userId.isEmpty())
                    {
                        channel->addMessage(makeSystemMessage(
                            "User " + target + " could not be followed!"));
                        return;
                    }
                    user->unfollowUser(userId, [channel, target]() {
                        channel->addMessage(makeSystemMessage(
                            "You successfully unfollowed " + target));
                    });
                });

            return "";
        }
        else if (commandName == "/logs")
        {
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage: /logs [user] (channel)"));
                return "";
            }
            auto app = getApp();

            auto logs = new LogsPopup();
            QString target = words.at(1);

            if (target.at(0) == '@')
            {
                target = target.mid(1);
            }

            logs->setTargetUserName(target);

            std::shared_ptr<Channel> logsChannel = channel;

            if (words.size() == 3)
            {
                QString channelName = words.at(2);
                if (words.at(2).at(0) == '#')
                {
                    channelName = channelName.mid(1);
                }

                logs->setChannelName(channelName);

                logsChannel =
                    app->twitch.server->getChannelOrEmpty(channelName);
            }

            logs->setChannel(logsChannel);

            logs->getLogs();
            logs->setAttribute(Qt::WA_DeleteOnClose);
            logs->show();
            return "";
        }
        else if (commandName == "/user")
        {
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage /user [user] (channel)"));
                return "";
            }
            QString channelName = channel->getName();
            if (words.size() > 2)
            {
                channelName = words[2];
                if (channelName[0] == '#')
                {
                    channelName.remove(0, 1);
                }
            }
            QDesktopServices::openUrl("https://www.twitch.tv/popout/" +
                                      channelName + "/viewercard/" + words[1]);
            return "";
        }
        else if (commandName == "/usercard")
        {
            if (words.size() < 2)
            {
                channel->addMessage(
                    makeSystemMessage("Usage /usercard [user]"));
                return "";
            }
            auto *userPopup = new UserInfoPopup;
            userPopup->setData(words[1], channel);
            userPopup->setActionOnFocusLoss(BaseWindow::Delete);
            userPopup->move(QCursor::pos());
            userPopup->show();
            return "";
        }
    }

    {
        // check if custom command exists
        const auto it = this->commandsMap_.find(commandName);
        if (it != this->commandsMap_.end())
        {
            return this->execCustomCommand(words, it.value(), dryRun);
        }
    }

    auto maxSpaces = std::min(this->maxSpaces_, words.length() - 1);
    for (int i = 0; i < maxSpaces; ++i)
    {
        commandName += ' ' + words[i + 1];

        const auto it = this->commandsMap_.find(commandName);
        if (it != this->commandsMap_.end())
        {
            return this->execCustomCommand(words, it.value(), dryRun);
        }
    }

    return text;
}

QString CommandController::execCustomCommand(const QStringList &words,
                                             const Command &command,
                                             bool dryRun)
{
    QString result;

    static QRegularExpression parseCommand("(^|[^{])({{)*{(\\d+\\+?)}");

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

QStringList CommandController::getDefaultTwitchCommandList()
{
    QStringList l = TWITCH_DEFAULT_COMMANDS;
    l += "/uptime";

    return l;
}

}  // namespace chatterino
