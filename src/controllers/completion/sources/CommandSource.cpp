// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/completion/sources/CommandSource.hpp"

#include "Application.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/completion/sources/Helpers.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

namespace chatterino::completion {

namespace {

void addCommand(const QString &command, std::vector<CommandItem> &out)
{
    CommandItem item;
    if (command.startsWith('/') || command.startsWith('.'))
    {
        item = {
            .name = command.mid(1),
            .prefix = command.at(0),
        };
    }
    else
    {
        item = {
            .name = command,
            .prefix = "",
        };
    }

    const auto duplicate =
        std::ranges::find_if(out, [&item](const auto &existing) {
            return existing.name == item.name && existing.prefix == item.prefix;
        });
    if (duplicate == out.end())
    {
        out.push_back(std::move(item));
    }
}

}  // namespace

CommandSource::CommandSource(std::unique_ptr<CommandStrategy> strategy,
                             ActionCallback callback)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
{
    this->initializeItems();
}

void CommandSource::update(const QString &query)
{
    this->output_.clear();
    if (this->strategy_)
    {
        this->strategy_->apply(this->items_, this->output_, query);
    }
}

void CommandSource::addToListModel(GenericListModel &model,
                                   size_t maxCount) const
{
    addVecToListModel(
        this->output_, model, maxCount, [this](const CommandItem &command) {
            return std::make_unique<InputCompletionItem>(
                nullptr, command.prefix + command.name, this->callback_);
        });
}

void CommandSource::addToStringList(QStringList &list, size_t maxCount,
                                    bool /* isFirstWord */) const
{
    addVecToStringList(this->output_, list, maxCount,
                       [](const CommandItem &command) {
                           return command.prefix + command.name + " ";
                       });
}

void CommandSource::initializeItems()
{
    std::vector<CommandItem> commands;

#ifdef CHATTERINO_HAVE_PLUGINS
    for (const auto &command : getApp()->getCommands()->pluginCommands())
    {
        addCommand(command, commands);
    }
#endif

    // Custom Chatterino commands
    for (const auto &command : getApp()->getCommands()->items)
    {
        addCommand(command.name, commands);
    }

    // Default Chatterino commands
    auto x = getApp()->getCommands()->getDefaultChatterinoCommandList();
    for (const auto &command : x)
    {
        addCommand(command, commands);
    }

    // Default Twitch commands
    for (const auto &command : TWITCH_DEFAULT_COMMANDS)
    {
        addCommand(command, commands);
    }

    this->items_ = std::move(commands);
}

const std::vector<CommandItem> &CommandSource::output() const
{
    return this->output_;
}

}  // namespace chatterino::completion
