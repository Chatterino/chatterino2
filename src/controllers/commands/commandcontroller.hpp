#pragma once

#include <QMap>
#include <memory>

#include "controllers/commands/command.hpp"
#include "util/signalvector2.hpp"

namespace chatterino {
class Channel;

namespace controllers {
namespace commands {

class CommandModel;

class CommandController
{
public:
    CommandController();

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel, bool dryRun);

    void load();
    void save();

    CommandModel *createModel(QObject *parent);

    util::UnsortedSignalVector<Command> items;

private:
    QMap<QString, Command> commandsMap;

    std::mutex mutex;
    QString filePath;

    QString execCustomCommand(const QStringList &words, const Command &command);
};

}  // namespace commands
}  // namespace controllers
}  // namespace chatterino
