#include "singletons/commandmanager.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "messages/messagebuilder.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/pathmanager.hpp"

#include <QApplication>
#include <QFile>
#include <QRegularExpression>

#include "channel.hpp"
#include "providers/twitch/twitchchannel.hpp"

using namespace chatterino::providers::twitch;

namespace chatterino {
namespace singletons {

CommandManager::CommandManager()
{
    auto addFirstMatchToMap = [this](auto args) {
        this->commandsMap.remove(args.item.name);

        for (const Command &cmd : this->commands.getVector()) {
            if (cmd.name == args.item.name) {
                this->commandsMap[cmd.name] = cmd;
                break;
            }
        }
    };

    this->commands.itemInserted.connect(addFirstMatchToMap);
    this->commands.itemRemoved.connect(addFirstMatchToMap);
}

void CommandManager::load()
{
    auto app = getApp();
    this->filePath = app->paths->customFolderPath + "/Commands.txt";

    QFile textFile(this->filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        // No commands file created yet
        return;
    }

    QList<QByteArray> test = textFile.readAll().split('\n');

    for (const auto &command : test) {
        this->commands.appendItem(Command(command));
    }

    textFile.close();
}

void CommandManager::save()
{
    QFile textFile(this->filePath);
    if (!textFile.open(QIODevice::WriteOnly)) {
        debug::Log("[CommandManager::saveCommands] Unable to open {} for writing", this->filePath);
        return;
    }

    for (const Command &cmd : this->commands.getVector()) {
        textFile.write((cmd.toString() + "\n").toUtf8());
    }

    textFile.close();
}

CommandModel *CommandManager::createModel(QObject *parent)
{
    return new CommandModel(&this->commands, parent);
}

QString CommandManager::execCommand(const QString &text, ChannelPtr channel, bool dryRun)
{
    QStringList words = text.split(' ', QString::SkipEmptyParts);
    Command command;

    {
        std::lock_guard<std::mutex> lock(this->mutex);

        if (words.length() == 0) {
            return text;
        }

        QString commandName = words[0];

        // check if default command exists
        auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

        if (!dryRun && twitchChannel != nullptr) {
            if (commandName == "/debug-args") {
                QString msg = QApplication::instance()->arguments().join(' ');

                channel->addMessage(messages::Message::createSystemMessage(msg));

                return "";
            } else if (commandName == "/uptime") {
                const auto &streamStatus = twitchChannel->GetStreamStatus();

                QString messageText =
                    streamStatus.live ? streamStatus.uptime : "Channel is not live.";

                channel->addMessage(messages::Message::createSystemMessage(messageText));

                return "";
            } else if (commandName == "/ignore" && words.size() >= 2) {
                // fourtf: ignore user
                //                QString messageText;

                //                if (IrcManager::getInstance().tryAddIgnoredUser(words.at(1),
                //                messageText)) {
                //                    messageText = "Ignored user \"" + words.at(1) + "\".";
                //                }

                //                channel->addMessage(messages::Message::createSystemMessage(messageText));
                return "";
            } else if (commandName == "/unignore") {
                // fourtf: ignore user
                //                QString messageText;

                //                if (IrcManager::getInstance().tryRemoveIgnoredUser(words.at(1),
                //                messageText)) {
                //                    messageText = "Ignored user \"" + words.at(1) + "\".";
                //                }

                //                channel->addMessage(messages::Message::createSystemMessage(messageText));
                return "";
            } else if (commandName == "/w") {
                if (words.length() <= 2) {
                    return "";
                }

                auto app = getApp();

                messages::MessageBuilder b;

                b.emplace<messages::TextElement>(app->accounts->Twitch.getCurrent()->getUserName(),
                                                 messages::MessageElement::Text);
                b.emplace<messages::TextElement>("->", messages::MessageElement::Text);
                b.emplace<messages::TextElement>(words[1], messages::MessageElement::Text);

                QString rest = "";

                for (int i = 2; i < words.length(); i++) {
                    rest += words[i];
                }

                b.emplace<messages::TextElement>(rest, messages::MessageElement::Text);

                app->twitch.server->whispersChannel->addMessage(b.getMessage());
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

QString CommandManager::execCustomCommand(const QStringList &words, const Command &command)
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

// commandmodel
CommandModel::CommandModel(util::BaseSignalVector<Command> *vec, QObject *parent)
    : util::SignalVectorModel<Command>(vec, 2, parent)
{
}

int CommandModel::prepareInsert(const Command &item, int index,
                                std::vector<QStandardItem *> &rowToAdd)
{
    rowToAdd[0]->setData(item.name, Qt::EditRole);
    rowToAdd[1]->setData(item.func, Qt::EditRole);

    return index;
}

int CommandModel::prepareRemove(const Command &item, int index)
{
    UNUSED(item);

    return index;
}

// command
Command::Command(const QString &_text)
{
    int index = _text.indexOf(' ');

    if (index == -1) {
        this->name = _text;
        return;
    }

    this->name = _text.mid(0, index);
    this->func = _text.mid(index + 1);
}

Command::Command(const QString &_name, const QString &_func)
    : name(_name)
    , func(_func)
{
}

QString Command::toString() const
{
    return this->name + " " + this->func;
}

}  // namespace singletons
}  // namespace chatterino
