#include "CommandController.hpp"

#include "Application.hpp"
#include "common/SignalVector.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "debug/Log.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchApi.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "widgets/dialogs/LogsPopup.hpp"

#include <QApplication>
#include <QFile>
#include <QRegularExpression>

#define TWITCH_DEFAULT_COMMANDS                                         \
    {                                                                   \
        "/help", "/w", "/me", "/disconnect", "/mods", "/color", "/ban", \
            "/unban", "/timeout", "/untimeout", "/slow", "/slowoff",    \
            "/r9kbeta", "/r9kbetaoff", "/emoteonly", "/emoteonlyoff",   \
            "/clear", "/subscribers", "/subscribersoff", "/followers",  \
            "/followersoff"                                             \
    }

namespace chatterino {

CommandController::CommandController()
{
    auto addFirstMatchToMap = [this](auto args) {
        this->commandsMap_.remove(args.item.name);

        for (const Command &cmd : this->items.getVector())
        {
            if (cmd.name == args.item.name)
            {
                this->commandsMap_[cmd.name] = cmd;
                break;
            }
        }
    };

    this->items.itemInserted.connect(addFirstMatchToMap);
    this->items.itemRemoved.connect(addFirstMatchToMap);
}

void CommandController::initialize(Settings &, Paths &paths)
{
    this->load(paths);
}

void CommandController::load(Paths &paths)
{
    this->filePath_ = combinePath(paths.settingsDirectory, "commands.txt");

    QFile textFile(this->filePath_);
    if (!textFile.open(QIODevice::ReadOnly))
    {
        // No commands file created yet
        return;
    }

    QList<QByteArray> test = textFile.readAll().split('\n');

    for (const auto &command : test)
    {
        if (command.isEmpty())
        {
            continue;
        }

        this->items.appendItem(Command(command));
    }

    textFile.close();
}

void CommandController::save()
{
    QFile textFile(this->filePath_);
    if (!textFile.open(QIODevice::WriteOnly))
    {
        log("[CommandController::saveCommands] Unable to open {} for writing",
            this->filePath_);
        return;
    }

    for (const Command &cmd : this->items.getVector())
    {
        textFile.write((cmd.toString() + "\n").toUtf8());
    }

    textFile.close();
}

CommandModel *CommandController::createModel(QObject *parent)
{
    CommandModel *model = new CommandModel(parent);
    model->init(&this->items);

    return model;
}

QString CommandController::execCommand(const QString &text, ChannelPtr channel,
                                       bool dryRun)
{
    QStringList words = text.split(' ', QString::SkipEmptyParts);
    Command command;

    {
        std::lock_guard<std::mutex> lock(this->mutex_);

        if (words.length() == 0)
        {
            return text;
        }

        QString commandName = words[0];

        // works in a valid twitch channel and /whispers, etc...
        if (!dryRun && channel->isTwitchChannel())
        {
            if (commandName == "/w")
            {
                if (words.length() <= 2)
                {
                    return "";
                }

                auto app = getApp();

                MessageBuilder b;

                b.emplace<TimestampElement>();
                b.emplace<TextElement>(
                    app->accounts->twitch.getCurrent()->getUserName(),
                    MessageElementFlag::Text, MessageColor::Text,
                    FontStyle::ChatMediumBold);
                b.emplace<TextElement>("->", MessageElementFlag::Text);
                b.emplace<TextElement>(words[1] + ":", MessageElementFlag::Text,
                                       MessageColor::Text,
                                       FontStyle::ChatMediumBold);

                const auto &acc = app->accounts->twitch.getCurrent();
                const auto &accemotes = *acc->accessEmotes();
                const auto &bttvemotes = app->twitch.server->getBttvEmotes();
                const auto &ffzemotes = app->twitch.server->getFfzEmotes();
                auto flags = MessageElementFlags();
                auto emote = boost::optional<EmotePtr>{};
                for (int i = 2; i < words.length(); i++) {
                    {  // twitch emote
                        auto it = accemotes.emotes.find({words[i]});
                        if (it != accemotes.emotes.end()) {
                            b.emplace<EmoteElement>(
                                it->second, MessageElementFlag::TwitchEmote);
                            continue;
                        }
                    }  // twitch emote
                    {  // bttv/ffz emote
                        {
                            if ((emote = bttvemotes.emote({words[i]}))) {
                                flags = MessageElementFlag::BttvEmote;
                            } else if ((emote = ffzemotes.emote({words[i]}))) {
                                flags = MessageElementFlag::FfzEmote;
                            }
                            if (emote) {
                                b.emplace<EmoteElement>(emote.get(), flags);
                                continue;
                            }
                        }  // bttv/ffz emote
                        {  // text
                            b.emplace<TextElement>(words[i],
                                                   MessageElementFlag::Text);
                        }  // text
                    }
                }

                b->flags.set(MessageFlag::DoNotTriggerNotification);
                auto messagexD = b.release();

                app->twitch.server->whispersChannel->addMessage(messagexD);

                app->twitch.server->sendMessage("jtv", text);

                if (getSettings()->inlineWhispers)
                {
                    app->twitch.server->forEachChannel(
                        [&messagexD](ChannelPtr _channel) {
                            _channel->addMessage(messagexD);
                        });
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

                QString messageText = streamStatus->live
                                          ? streamStatus->uptime
                                          : "Channel is not live.";

                channel->addMessage(makeSystemMessage(messageText));

                return "";
            }
            else if (commandName == "/ignore")
            {
                if (words.size() < 2)
                {
                    channel->addMessage(
                        makeSystemMessage("Usage: /ignore [user]"));
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

                user->ignore(
                    target, [channel](auto resultCode, const QString &message) {
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

                user->unignore(
                    target, [channel](auto resultCode, const QString &message) {
                        channel->addMessage(makeSystemMessage(message));
                    });

                return "";
            }
            else if (commandName == "/follow")
            {
                if (words.size() < 2)
                {
                    channel->addMessage(
                        makeSystemMessage("Usage: /follow [user]"));
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
                QString target;

                if (words.at(1).at(0) == "@")
                {
                    target = words.at(1).mid(1);
                }
                else
                {
                    target = words.at(1);
                }

                if (words.size() == 3)
                {
                    QString channelName = words.at(2);
                    if (words.at(2).at(0) == "#")
                    {
                        channelName = words.at(2).mid(1);
                    }
                    auto logsChannel =
                        app->twitch.server->getChannelOrEmpty(channelName);
                    if (logsChannel == nullptr)
                    {
                    }
                    else
                    {
                        logs->setInfo(logsChannel, target);
                    }
                }
                else
                {
                    logs->setInfo(channel, target);
                }
                logs->setAttribute(Qt::WA_DeleteOnClose);
                logs->show();
                return "";
            }
        }

        // check if custom command exists
        auto it = this->commandsMap_.find(commandName);
        if (it == this->commandsMap_.end())
        {
            return text;
        }

        command = it.value();
    }

    return this->execCustomCommand(words, command);
}

QString CommandController::execCustomCommand(const QStringList &words,
                                             const Command &command)
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

    return result.replace("{{", "{");
}

QStringList CommandController::getDefaultTwitchCommandList()
{
    QStringList l = TWITCH_DEFAULT_COMMANDS;
    l += "/uptime";

    return l;
}

}  // namespace chatterino
