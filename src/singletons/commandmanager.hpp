#pragma once

#include <QMap>
#include <QString>

#include <memory>
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
    friend class Application;

public:
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
    QString filePath;

    QString execCustomCommand(const QStringList &words, const Command &command);
};

}  // namespace singletons
}  // namespace chatterino
