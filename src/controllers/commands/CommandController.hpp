#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "controllers/commands/Command.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QMap>
#include <pajlada/settings.hpp>

#include <memory>
#include <mutex>
#include <unordered_map>

namespace chatterino {

class Settings;
class Paths;
class Channel;

class CommandModel;

class CommandController final : public Singleton
{
public:
    SignalVector<Command> items;

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel,
                        bool dryRun);
    QStringList getDefaultChatterinoCommandList();

    virtual void initialize(Settings &, Paths &paths) override;
    virtual void save() override;

    CommandModel *createModel(QObject *parent);

    QString execCustomCommand(
        const QStringList &words, const Command &command, bool dryRun,
        ChannelPtr channel, const Message *message = nullptr,
        std::unordered_map<QString, QString> context = {});

private:
    void load(Paths &paths);

    using CommandFunction =
        std::function<QString(QStringList /*words*/, ChannelPtr /*channel*/)>;

    void registerCommand(QString commandName, CommandFunction commandFunction);

    // Chatterino commands
    QMap<QString, CommandFunction> commands_;

    // User-created commands
    QMap<QString, Command> userCommands_;
    int maxSpaces_ = 0;

    std::shared_ptr<pajlada::Settings::SettingManager> sm_;
    // Because the setting manager is not initialized until the initialize
    // function is called (and not in the constructor), we have to
    // late-initialize the setting, which is why we're storing it as a
    // unique_ptr
    std::unique_ptr<pajlada::Settings::Setting<std::vector<Command>>>
        commandsSetting_;

    QStringList defaultChatterinoCommandAutoCompletions_;
};

}  // namespace chatterino
