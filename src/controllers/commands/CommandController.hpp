#pragma once

#include <QMap>
#include <memory>
#include <mutex>

#include "common/SignalVector.hpp"
#include "controllers/commands/Command.hpp"

namespace chatterino {
class Channel;

class CommandModel;

class CommandController
{
public:
    CommandController();

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel, bool dryRun);
    QStringList getDefaultTwitchCommandList();

    void load();
    void save();

    CommandModel *createModel(QObject *parent);

    UnsortedSignalVector<Command> items;

private:
    QMap<QString, Command> commandsMap_;

    std::mutex mutex_;
    QString filePath_;

    QString execCustomCommand(const QStringList &words, const Command &command);
};

}  // namespace chatterino
