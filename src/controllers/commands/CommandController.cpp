#include "CommandController.hpp"

#include "Application.hpp"
#include "common/SignalVector2.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/PathManager.hpp"
#include "singletons/SettingsManager.hpp"

#include <QApplication>
#include <QFile>
#include <QRegularExpression>

#define TWITCH_DEFAULT_COMMANDS                                                               \
    {                                                                                         \
        "/help", "/w", "/me", "/disconnect", "/mods", "/color", "/ban", "/unban", "/timeout", \
            "/untimeout", "/slow", "/slowoff", "/r9kbeta", "/r9kbetaoff", "/emoteonly",       \
            "/emoteonlyoff", "/clear", "/subscribers", "/subscribersoff", "/followers",       \
            "/followersoff"                                                                   \
    }

namespace chatterino {

CommandController::CommandController()
{
    auto addFirstMatchToMap = [this](auto args) {
        this->commandsMap.remove(args.item.name);

        for (const Command &cmd : this->items.getVector()) {
            if (cmd.name == args.item.name) {
                this->commandsMap[cmd.name] = cmd;
                break;
            }
        }
    };

    this->items.itemInserted.connect(addFirstMatchToMap);
    this->items.itemRemoved.connect(addFirstMatchToMap);
}

void CommandController::load()
{
    auto app = getApp();
    this->filePath = app->paths->settingsDirectory + "/commands.txt";

    QFile textFile(this->filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        // No commands file created yet
        return;
    }

    QList<QByteArray> test = textFile.readAll().split('\n');

    for (const auto &command : test) {
        if (command.isEmpty()) {
            continue;
        }

        this->items.appendItem(Command(command));
    }

    textFile.close();
}

void CommandController::save()
{
    QFile textFile(this->filePath);
    if (!textFile.open(QIODevice::WriteOnly)) {
        Log("[CommandController::saveCommands] Unable to open {} for writing", this->filePath);
        return;
    }

    for (const Command &cmd : this->items.getVector()) {
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

QString CommandController::execCommand(const QString &text, ChannelPtr channel, bool dryRun)
{
    QStringList words = text.split(' ', QString::SkipEmptyParts);
    Command command;

    {
        std::lock_guard<std::mutex> lock(this->mutex);

        if (words.length() == 0) {
            return text;
        }

        QString commandName = words[0];

        // works in a valid twitch channel and /whispers, etc...
        if (!dryRun && channel->isTwitchChannel()) {
            if (commandName == "/w") {
                if (words.length() <= 2) {
                    return "";
                }

                auto app = getApp();

                MessageBuilder b;

                b.emplace<TimestampElement>();
                b.emplace<TextElement>(app->accounts->twitch.getCurrent()->getUserName(),
                                       MessageElement::Text, MessageColor::Text,
                                       FontStyle::ChatMediumBold);
                b.emplace<TextElement>("->", MessageElement::Text);
                b.emplace<TextElement>(words[1] + ":", MessageElement::Text, MessageColor::Text,
                                       FontStyle::ChatMediumBold);

                QString rest = "";

                for (int i = 2; i < words.length(); i++) {
                    rest += words[i] + " ";
                }

                b.emplace<TextElement>(rest, MessageElement::Text);
                b.getMessage()->flags |= Message::DoNotTriggerNotification;

                app->twitch.server->whispersChannel->addMessage(b.getMessage());

                app->twitch.server->getWriteConnection()->sendRaw("PRIVMSG #jtv :" + text + "\r\n");

                if (app->settings->inlineWhispers) {
                    app->twitch.server->forEachChannel(
                        [&b](ChannelPtr _channel) { _channel->addMessage(b.getMessage()); });
                }

                return "";
            }
        }

        // check if default command exists
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

        // works only in a valid twitch channel
        if (!dryRun && twitchChannel != nullptr) {
            if (commandName == "/debug-args") {
                QString msg = QApplication::instance()->arguments().join(' ');

                channel->addMessage(Message::createSystemMessage(msg));

                return "";
            } else if (commandName == "/uptime") {
                const auto &streamStatus = twitchChannel->getStreamStatus();

                QString messageText =
                    streamStatus.live ? streamStatus.uptime : "Channel is not live.";

                channel->addMessage(Message::createSystemMessage(messageText));

                return "";
            } else if (commandName == "/ignore" && words.size() >= 2) {
                auto app = getApp();

                auto user = app->accounts->twitch.getCurrent();
                auto target = words.at(1);

                if (user->isAnon()) {
                    channel->addMessage(Message::createSystemMessage(
                        "You must be logged in to ignore someone"));
                    return "";
                }

                user->ignore(target, [channel](auto resultCode, const QString &message) {
                    channel->addMessage(Message::createSystemMessage(message));
                });

                return "";
            } else if (commandName == "/unignore" && words.size() >= 2) {
                auto app = getApp();

                auto user = app->accounts->twitch.getCurrent();
                auto target = words.at(1);

                if (user->isAnon()) {
                    channel->addMessage(Message::createSystemMessage(
                        "You must be logged in to ignore someone"));
                    return "";
                }

                user->unignore(target, [channel](auto resultCode, const QString &message) {
                    channel->addMessage(Message::createSystemMessage(message));
                });

                return "";
            }
        }

        // check if custom command exists
        auto it = this->commandsMap.find(commandName);
        if (it == this->commandsMap.end()) {
            return text;
        }

        command = it.value();
    }

    return this->execCustomCommand(words, command);
}

QString CommandController::execCustomCommand(const QStringList &words, const Command &command)
{
    QString result;

    static QRegularExpression parseCommand("(^|[^{])({{)*{(\\d+\\+?)}");

    int lastCaptureEnd = 0;

    auto globalMatch = parseCommand.globalMatch(command.func);
    int matchOffset = 0;

    while (true) {
        QRegularExpressionMatch match = parseCommand.match(command.func, matchOffset);

        if (!match.hasMatch()) {
            break;
        }

        result += command.func.mid(lastCaptureEnd, match.capturedStart() - lastCaptureEnd + 1);

        lastCaptureEnd = match.capturedEnd();
        matchOffset = lastCaptureEnd - 1;

        QString wordIndexMatch = match.captured(3);

        bool plus = wordIndexMatch.at(wordIndexMatch.size() - 1) == '+';
        wordIndexMatch = wordIndexMatch.replace("+", "");

        bool ok;
        int wordIndex = wordIndexMatch.replace("=", "").toInt(&ok);
        if (!ok || wordIndex == 0) {
            result += "{" + match.captured(3) + "}";
            continue;
        }

        if (words.length() <= wordIndex) {
            continue;
        }

        if (plus) {
            bool first = true;
            for (int i = wordIndex; i < words.length(); i++) {
                if (!first) {
                    result += " ";
                }
                result += words[i];
                first = false;
            }
        } else {
            result += words[wordIndex];
        }
    }

    result += command.func.mid(lastCaptureEnd);

    if (result.size() > 0 && result.at(0) == '{') {
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
