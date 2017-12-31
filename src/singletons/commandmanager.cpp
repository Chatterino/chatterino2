#include "singletons/commandmanager.hpp"

#include <QRegularExpression>

namespace chatterino {
namespace singletons {
CommandManager &CommandManager::getInstance()
{
    static CommandManager instance;
    return instance;
}

// QString CommandManager::execCommand(QString text)
//{
//    QStringList words = text.split(' ', QString::SkipEmptyParts);

//    if (words.length() == 0) {
//        return text;
//    }

//    QString commandName = words[0];
//    if (commandName[0] == "/") {
//        commandName = commandName.mid(1);
//    }

//    Command *command = nullptr;
//    for (Command &command : this->commands) {
//        if (command.name == commandName) {
//            command = &command;
//            break;
//        }
//    }

//    if (command == nullptr) {
//        return text;
//    }

//    QString result;

//    static QRegularExpression parseCommand("[^{]({{)*{(\d+\+?)}");
//    for (QRegularExpressionMatch &match : parseCommand.globalMatch(command->text)) {
//        result += text.mid(match.capturedStart(), match.capturedLength());

//        QString wordIndexMatch = match.captured(2);

//        bool ok;
//        int wordIndex = wordIndexMatch.replace("=", "").toInt(ok);
//        if (!ok) {
//            result += match.captured();
//            continue;
//        }
//        if (words.length() <= wordIndex) {
//            // alternatively return text because the operation failed
//            result += "";
//            return;
//        }

//        if (wordIndexMatch[wordIndexMatch.length() - 1] == '+') {
//            for (int i = wordIndex; i < words.length(); i++) {
//                result += words[i];
//            }
//        } else {
//            result += words[wordIndex];
//        }
//    }

//    result += text.mid(match.capturedStart(), match.capturedLength());

//    return result;
//}

// CommandManager::Command::Command(QString _text)
//{
//    int index = _text.indexOf(' ');

//    if (index == -1) {
//        this->name == _text;
//        return;
//    }

//    this->name = _text.mid(0, index);
//    this->text = _text.mid(index + 1);
//}
}
}
