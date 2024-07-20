#pragma once

#include "common/SignalVector.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/settings.hpp>
#include <QMap>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <variant>

namespace chatterino {

class Settings;
class Paths;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
struct Message;

struct Command;
class CommandModel;
struct CommandContext;

class CommandController final
{
public:
    SignalVector<Command> items;

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel,
                        bool dryRun);
    QStringList getDefaultChatterinoCommandList();

    CommandController(const Paths &paths);
    void save();

    CommandModel *createModel(QObject *parent);

    QString execCustomCommand(
        const QStringList &words, const Command &command, bool dryRun,
        ChannelPtr channel, const Message *message = nullptr,
        std::unordered_map<QString, QString> context = {});
#ifdef CHATTERINO_HAVE_PLUGINS
    bool registerPluginCommand(const QString &commandName);
    bool unregisterPluginCommand(const QString &commandName);

    const QStringList &pluginCommands()
    {
        return this->pluginCommands_;
    }
#endif

private:
    void load(Paths &paths);

    using CommandFunction =
        std::function<QString(QStringList /*words*/, ChannelPtr /*channel*/)>;

    using CommandFunctionWithContext = std::function<QString(CommandContext)>;

    using CommandFunctionVariants =
        std::variant<CommandFunction, CommandFunctionWithContext>;

    void registerCommand(const QString &commandName,
                         CommandFunctionVariants commandFunction);

    // Chatterino commands
    std::unordered_map<QString, CommandFunctionVariants> commands_;

    // User-created commands
    QMap<QString, Command> userCommands_;
    qsizetype maxSpaces_ = 0;

    std::shared_ptr<pajlada::Settings::SettingManager> sm_;
    // Because the setting manager is not initialized until the initialize
    // function is called (and not in the constructor), we have to
    // late-initialize the setting, which is why we're storing it as a
    // unique_ptr
    std::unique_ptr<pajlada::Settings::Setting<std::vector<Command>>>
        commandsSetting_;

    QStringList defaultChatterinoCommandAutoCompletions_;
#ifdef CHATTERINO_HAVE_PLUGINS
    QStringList pluginCommands_;
#endif
};

}  // namespace chatterino
