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

void CommandManager::loadCommands()
{
    auto app = getApp();
    this->filePath = app->paths->customFolderPath + "/Commands.txt";

    QFile textFile(this->filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        // No commands file created yet
        return;
    }

    QList<QByteArray> test = textFile.readAll().split('\n');

    QStringList loadedCommands;

    for (const auto &command : test) {
        loadedCommands.append(command);
    }

    this->setCommands(loadedCommands);

    textFile.close();
}

void CommandManager::saveCommands()
{
    QFile textFile(this->filePath);
    if (!textFile.open(QIODevice::WriteOnly)) {
        debug::Log("[CommandManager::saveCommands] Unable to open {} for writing", this->filePath);
        return;
    }

    QString commandsString = this->commandsStringList.join('\n');

    textFile.write(commandsString.toUtf8());

    textFile.close();
}

void CommandManager::setCommands(const QStringList &_commands)
{
    std::lock_guard<std::mutex> lock(this->mutex);

    this->commands.clear();

    for (const QString &commandRef : _commands) {
        QString command = commandRef;

        if (command.size() == 0) {
            continue;
        }

        //        if (command.at(0) != '/') {
        //            command = QString("/") + command;
        //        }

        QString commandName = command.mid(0, command.indexOf(' '));

        if (this->commands.find(commandName) == this->commands.end()) {
            this->commands.insert(commandName, Command(command));
        }
    }

    this->commandsStringList = _commands;
    this->commandsStringList.detach();
}

QStringList CommandManager::getCommands()
{
    return this->commandsStringList;
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

                messages::MessageBuilder b;

                b.emplace<messages::TextElement>(
                    getApp()->accounts->Twitch.getCurrent()->getUserName(),
                    messages::MessageElement::Text);
                b.emplace<messages::TextElement>("->", messages::MessageElement::Text);
                b.emplace<messages::TextElement>(words[1], messages::MessageElement::Text);

                QString rest = "";

                for (int i = 2; i < words.length(); i++) {
                    rest += words[i];
                }

                b.emplace<messages::TextElement>(rest, messages::MessageElement::Text);

                TwitchServer::getInstance().whispersChannel->addMessage(b.getMessage());
            }
        }

        // check if custom command exists
        auto it = this->commands.find(commandName);

        if (it == this->commands.end()) {
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

    auto globalMatch = parseCommand.globalMatch(command.text);
    int matchOffset = 0;

    while (true) {
        QRegularExpressionMatch match = parseCommand.match(command.text, matchOffset);

        if (!match.hasMatch()) {
            break;
        }

        result += command.text.mid(lastCaptureEnd, match.capturedStart() - lastCaptureEnd + 1);

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

    result += command.text.mid(lastCaptureEnd);

    if (result.size() > 0 && result.at(0) == '{') {
        result = result.mid(1);
    }

    return result.replace("{{", "{");
}

CommandManager::Command::Command(QString _text)
{
    int index = _text.indexOf(' ');

    if (index == -1) {
        this->name = _text;
        return;
    }

    this->name = _text.mid(0, index);
    this->text = _text.mid(index + 1);
}

}  // namespace singletons
}  // namespace chatterino
