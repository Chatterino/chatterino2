#include "providers/autocomplete/AutocompleteSources.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

namespace chatterino {

namespace {

    void addEmotes(std::vector<CompletionEmote> &out, const EmoteMap &map,
                   const QString &providerName)
    {
        for (auto &&emote : map)
        {
            out.push_back({.emote = emote.second,
                           .searchName = emote.first.string,
                           .tabCompletionName = emote.first.string,
                           .displayName = emote.second->name.string,
                           .providerName = providerName});
        }
    }

    void addEmojis(std::vector<CompletionEmote> &out, const EmojiMap &map)
    {
        map.each([&](const QString &, const std::shared_ptr<EmojiData> &emoji) {
            for (auto &&shortCode : emoji->shortCodes)
            {
                out.push_back(
                    {.emote = emoji->emote,
                     .searchName = shortCode,
                     .tabCompletionName = QStringLiteral(":%1:").arg(shortCode),
                     .displayName = shortCode,
                     .providerName = "Emoji"});
            }
        });
    }

    void addCommand(const QString &command, std::vector<CompletionCommand> &out)
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

//// AutocompleteEmoteSource

AutocompleteEmoteSource::AutocompleteEmoteSource(
    const Channel &channel, std::unique_ptr<AutocompleteEmoteStrategy> strategy,
    ActionCallback callback)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
{
    this->initializeFromChannel(&channel);
}

void AutocompleteEmoteSource::update(const QString &query)
{
    this->output_.clear();
    if (this->strategy_)
    {
        this->strategy_->apply(this->items_, this->output_, query);
    }
}

void AutocompleteEmoteSource::copyToListModel(GenericListModel &model,
                                              size_t maxCount) const
{
    size_t count = maxCount == 0 ? this->output_.size()
                                 : std::min(this->output_.size(), maxCount);

    model.clear();
    model.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        model.addItem(this->mapListItem(this->output_[i]));
    }
}

void AutocompleteEmoteSource::copyToStringModel(QStringListModel &model,
                                                size_t maxCount,
                                                bool isFirstWord) const
{
    size_t count = maxCount == 0 ? this->output_.size()
                                 : std::min(this->output_.size(), maxCount);

    QStringList newData;
    newData.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        newData.push_back(
            this->mapTabStringItem(this->output_[i], isFirstWord));
    }

    model.setStringList(newData);
}

void AutocompleteEmoteSource::initializeFromChannel(const Channel *channel)
{
    auto *app = getIApp();

    std::vector<CompletionEmote> emotes;
    const auto *tc = dynamic_cast<const TwitchChannel *>(channel);
    // returns true also for special Twitch channels (/live, /mentions, /whispers, etc.)
    if (channel->isTwitchChannel())
    {
        if (auto user = app->getAccounts()->twitch.getCurrent())
        {
            // Twitch Emotes available globally
            auto emoteData = user->accessEmotes();
            addEmotes(emotes, emoteData->emotes, "Twitch Emote");

            // Twitch Emotes available locally
            auto localEmoteData = user->accessLocalEmotes();
            if (tc &&
                localEmoteData->find(tc->roomId()) != localEmoteData->end())
            {
                if (const auto *localEmotes = &localEmoteData->at(tc->roomId()))
                {
                    addEmotes(emotes, *localEmotes, "Local Twitch Emotes");
                }
            }
        }

        if (tc)
        {
            // TODO extract "Channel {BetterTTV,7TV,FrankerFaceZ}" text into a #define.
            if (auto bttv = tc->bttvEmotes())
            {
                addEmotes(emotes, *bttv, "Channel BetterTTV");
            }
            if (auto ffz = tc->ffzEmotes())
            {
                addEmotes(emotes, *ffz, "Channel FrankerFaceZ");
            }
            if (auto seventv = tc->seventvEmotes())
            {
                addEmotes(emotes, *seventv, "Channel 7TV");
            }
        }

        if (auto bttvG = app->getTwitch()->getBttvEmotes().emotes())
        {
            addEmotes(emotes, *bttvG, "Global BetterTTV");
        }
        if (auto ffzG = app->getTwitch()->getFfzEmotes().emotes())
        {
            addEmotes(emotes, *ffzG, "Global FrankerFaceZ");
        }
        if (auto seventvG = app->getTwitch()->getSeventvEmotes().globalEmotes())
        {
            addEmotes(emotes, *seventvG, "Global 7TV");
        }
    }

    addEmojis(emotes, app->getEmotes()->getEmojis()->getEmojis());

    this->items_ = std::move(emotes);
}

std::unique_ptr<GenericListItem> AutocompleteEmoteSource::mapListItem(
    const CompletionEmote &emote) const
{
    return std::make_unique<InputCompletionItem>(
        emote.emote, emote.displayName + " - " + emote.providerName,
        this->callback_);
}

QString AutocompleteEmoteSource::mapTabStringItem(const CompletionEmote &emote,
                                                  bool /* isFirstWord */) const
{
    return emote.tabCompletionName + " ";
}

//// AutocompleteUsersSource

AutocompleteUsersSource::AutocompleteUsersSource(
    const Channel &channel, std::unique_ptr<AutocompleteUsersStrategy> strategy,
    ActionCallback callback)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
{
    this->initializeFromChannel(&channel);
}

void AutocompleteUsersSource::update(const QString &query)
{
    this->output_.clear();
    if (this->strategy_)
    {
        this->strategy_->apply(this->items_, this->output_, query);
    }
}

void AutocompleteUsersSource::copyToListModel(GenericListModel &model,
                                              size_t maxCount) const
{
    size_t count = maxCount == 0 ? this->output_.size()
                                 : std::min(this->output_.size(), maxCount);

    model.clear();
    model.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        model.addItem(this->mapListItem(this->output_[i]));
    }
}

void AutocompleteUsersSource::copyToStringModel(QStringListModel &model,
                                                size_t maxCount,
                                                bool isFirstWord) const
{
    size_t count = maxCount == 0 ? this->output_.size()
                                 : std::min(this->output_.size(), maxCount);

    QStringList newData;
    newData.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        newData.push_back(
            this->mapTabStringItem(this->output_[i], isFirstWord));
    }

    model.setStringList(newData);
}

void AutocompleteUsersSource::initializeFromChannel(const Channel *channel)
{
    const auto *tc = dynamic_cast<const TwitchChannel *>(channel);
    if (!tc)
    {
        return;
    }

    this->items_ = tc->accessChatters()->all();
}

std::unique_ptr<GenericListItem> AutocompleteUsersSource::mapListItem(
    const UsersAutocompleteItem &user) const
{
    return std::make_unique<InputCompletionItem>(nullptr, user.second,
                                                 this->callback_);
}

QString AutocompleteUsersSource::mapTabStringItem(
    const UsersAutocompleteItem &user, bool isFirstWord) const
{
    const auto userMention = formatUserMention(
        user.second, isFirstWord, getSettings()->mentionUsersWithComma);
    return "@" + userMention + " ";
}

//// AutocompleteCommandsSource

AutocompleteCommandsSource::AutocompleteCommandsSource(
    std::unique_ptr<AutocompleteCommandStrategy> strategy,
    ActionCallback callback)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
{
    this->initializeItems();
}

void AutocompleteCommandsSource::update(const QString &query)
{
    this->output_.clear();
    if (this->strategy_)
    {
        this->strategy_->apply(this->items_, this->output_, query);
    }
}

void AutocompleteCommandsSource::copyToListModel(GenericListModel &model,
                                                 size_t maxCount) const
{
    size_t count = maxCount == 0 ? this->output_.size()
                                 : std::min(this->output_.size(), maxCount);

    model.clear();
    model.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        model.addItem(this->mapListItem(this->output_[i]));
    }
}

void AutocompleteCommandsSource::copyToStringModel(QStringListModel &model,
                                                   size_t maxCount,
                                                   bool /* isFirstWord */) const
{
    size_t count = maxCount == 0 ? this->output_.size()
                                 : std::min(this->output_.size(), maxCount);

    QStringList newData;
    newData.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        newData.push_back(this->mapTabStringItem(this->output_[i]));
    }

    model.setStringList(newData);
}

void AutocompleteCommandsSource::initializeItems()
{
    std::vector<CompletionCommand> commands;

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

    this->items_ = std::move(commands);
}

std::unique_ptr<GenericListItem> AutocompleteCommandsSource::mapListItem(
    const CompletionCommand &command) const
{
    return std::make_unique<InputCompletionItem>(nullptr, command.name,
                                                 this->callback_);
}

QString AutocompleteCommandsSource::mapTabStringItem(
    const CompletionCommand &command) const
{
    return command.prefix + command.name + " ";
}

}  // namespace chatterino
