#pragma once

#include "common/Singleton.hpp"

#include <QMap>
#include <memory>
#include <mutex>

#include "common/SignalVector.hpp"
#include "controllers/commands/Command.hpp"

namespace chatterino {

class Settings;
class Paths;
class Channel;

class CommandModel;

class CommandController final : public Singleton
{
public:
    CommandController();

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel,
                        bool dryRun);
    QStringList getDefaultTwitchCommandList();

    virtual void initialize(Settings &settings, Paths &paths) override;
    virtual void save() override;

    CommandModel *createModel(QObject *parent);

    UnsortedSignalVector<Command> items;

private:
    void load(Paths &paths);

    QMap<QString, Command> commandsMap_;

    std::mutex mutex_;
    QString filePath_;

    QString execCustomCommand(const QStringList &words, const Command &command);
};

}  // namespace chatterino
