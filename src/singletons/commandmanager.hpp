#pragma once

#include <QMap>
#include <QString>
#include <mutex>

namespace chatterino {
class Channel;

namespace singletons {

//
// this class managed the custom /commands
//

class CommandManager
{
    CommandManager() = default;

public:
    static CommandManager &getInstance();

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel, bool dryRun);

    void loadCommands();
    void saveCommands();

    void setCommands(const QStringList &commands);
    QStringList getCommands();

private:
    struct Command {
        QString name;
        QString text;

        Command() = default;
        Command(QString text);
    };

    QMap<QString, Command> commands;
    std::mutex mutex;
    QStringList commandsStringList;

    QString execCustomCommand(const QStringList &words, const Command &command);
};
}
}
