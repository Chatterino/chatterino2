#include "providers/autocomplete/AutocompleteCommandsSource.hpp"

#include "Application.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

namespace chatterino {

namespace {
    void addCommand(const QString &command, std::vector<CompleteCommand> &out)
    {
        if (command.startsWith('/') || command.startsWith('.'))
        {
            out.push_back({command.mid(1), command.at(0)});
        }
        else
        {
            out.push_back({command, '/'});
        }
    }
}  // namespace

AutocompleteCommandsSource::AutocompleteCommandsSource(
    ActionCallback callback,
    std::unique_ptr<AutocompleteCommandStrategy> strategy)
    : AutocompleteGenericSource({}, std::move(strategy))  // begin with no items
    , callback_(std::move(callback))
{
    this->initializeItems();
}

void AutocompleteCommandsSource::initializeItems()
{
    std::vector<CompleteCommand> commands;

#ifdef CHATTERINO_HAVE_PLUGINS
    for (const auto &command : getApp()->commands->pluginCommands())
    {
        addCommand(command, commands);
    }
#endif

    // Custom Chatterino commands
    for (const auto &command : getIApp()->getCommands()->items)
    {
        addCommand(command.name, commands);
    }

    // Default Chatterino commands
    auto x = getIApp()->getCommands()->getDefaultChatterinoCommandList();
    for (const auto &command : x)
    {
        addCommand(command, commands);
    }

    // Default Twitch commands
    for (const auto &command : TWITCH_DEFAULT_COMMANDS)
    {
        addCommand(command, commands);
    }

    this->setItems(std::move(commands));
}

std::unique_ptr<GenericListItem> AutocompleteCommandsSource::mapListItem(
    const CompleteCommand &command) const
{
    return std::make_unique<InputCompletionItem>(nullptr, command.name,
                                                 this->callback_);
}

QString AutocompleteCommandsSource::mapTabStringItem(
    const CompleteCommand &command, bool isFirstWord) const
{
    return command.prefix + command.name + " ";
}

}  // namespace chatterino
