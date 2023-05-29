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

    size_t sizeWithinLimit(size_t size, size_t limit)
    {
        if (limit == 0)
        {
            return size;
        }
        return std::min(size, limit);
    }

    template <typename T, typename Mapper>
    void addVecToListModel(const std::vector<T> &input, GenericListModel &model,
                           size_t maxCount, Mapper mapper)
    {
        const size_t count = sizeWithinLimit(input.size(), maxCount);
        model.reserve(model.rowCount() + count);

        for (size_t i = 0; i < count; ++i)
        {
            model.addItem(mapper(input[i]));
        }
    }

    template <typename T, typename Mapper>
    void addVecToStringList(const std::vector<T> &input, QStringList &list,
                            size_t maxCount, Mapper mapper)
    {
        const size_t count = sizeWithinLimit(input.size(), maxCount);
        list.reserve(list.count() + count);

        for (size_t i = 0; i < count; ++i)
        {
            list.push_back(mapper(input[i]));
        }
    }

    void addEmotes(std::vector<CompletionEmote> &out, const EmoteMap &map,
                   const QString &providerName)
    {
        for (auto &&emote : map)
        {
            out.push_back({.emote = emote.second,
                           .searchName = emote.first.string,
                           .tabCompletionName = emote.first.string,
                           .displayName = emote.second->name.string,
                           .providerName = providerName,
                           .isEmoji = false});
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
                     .providerName = "Emoji",
                     .isEmoji = true});
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

void AutocompleteEmoteSource::addToListModel(GenericListModel &model,
                                             size_t maxCount) const
{
    addVecToListModel(this->output_, model, maxCount,
                      [this](const CompletionEmote &e) {
                          return std::make_unique<InputCompletionItem>(
                              e.emote, e.displayName + " - " + e.providerName,
                              this->callback_);
                      });
}

void AutocompleteEmoteSource::addToStringList(QStringList &list,
                                              size_t maxCount,
                                              bool /* isFirstWord */) const
{
    addVecToStringList(this->output_, list, maxCount,
                       [](const CompletionEmote &e) {
                           return e.tabCompletionName + " ";
                       });
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
            if ((tc != nullptr) &&
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

const std::vector<CompletionEmote> &AutocompleteEmoteSource::output() const
{
    return this->output_;
}

//// AutocompleteUsersSource

AutocompleteUsersSource::AutocompleteUsersSource(
    const Channel &channel, std::unique_ptr<AutocompleteUsersStrategy> strategy,
    ActionCallback callback, bool prependAt)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
    , prependAt_(prependAt)
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

void AutocompleteUsersSource::addToListModel(GenericListModel &model,
                                             size_t maxCount) const
{
    addVecToListModel(this->output_, model, maxCount,
                      [this](const UsersAutocompleteItem &user) {
                          return std::make_unique<InputCompletionItem>(
                              nullptr, user.second, this->callback_);
                      });
}

void AutocompleteUsersSource::addToStringList(QStringList &list,
                                              size_t maxCount,
                                              bool isFirstWord) const
{
    bool mentionComma = getSettings()->mentionUsersWithComma;
    addVecToStringList(
        this->output_, list, maxCount,
        [this, isFirstWord, mentionComma](const UsersAutocompleteItem &user) {
            const auto userMention =
                formatUserMention(user.second, isFirstWord, mentionComma);
            QString strTemplate = this->prependAt_ ? QStringLiteral("@%1 ")
                                                   : QStringLiteral("%1 ");
            return strTemplate.arg(userMention);
        });
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

const std::vector<UsersAutocompleteItem> &AutocompleteUsersSource::output()
    const
{
    return this->output_;
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

void AutocompleteCommandsSource::addToListModel(GenericListModel &model,
                                                size_t maxCount) const
{
    addVecToListModel(this->output_, model, maxCount,
                      [this](const CompletionCommand &command) {
                          return std::make_unique<InputCompletionItem>(
                              nullptr, command.name, this->callback_);
                      });
}

void AutocompleteCommandsSource::addToStringList(QStringList &list,
                                                 size_t maxCount,
                                                 bool /* isFirstWord */) const
{
    addVecToStringList(this->output_, list, maxCount,
                       [](const CompletionCommand &command) {
                           return command.prefix + command.name + " ";
                       });
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

const std::vector<CompletionCommand> &AutocompleteCommandsSource::output() const
{
    return this->output_;
}

//// AutocompleteUnifiedSource

AutocompleteUnifiedSource::AutocompleteUnifiedSource(
    const Channel &channel,
    std::unique_ptr<AutocompleteEmoteSource::AutocompleteEmoteStrategy>
        emoteStrategy,
    std::unique_ptr<AutocompleteUsersSource::AutocompleteUsersStrategy>
        userStrategy,
    ActionCallback callback)
    : emoteSource_(channel, std::move(emoteStrategy), callback)
    , usersSource_(channel, std::move(userStrategy), callback,
                   false)  // disable adding @ to front
{
}

void AutocompleteUnifiedSource::update(const QString &query)
{
    this->emoteSource_.update(query);
    this->usersSource_.update(query);
}

void AutocompleteUnifiedSource::addToListModel(GenericListModel &model,
                                               size_t maxCount) const
{
    if (maxCount == 0)
    {
        this->emoteSource_.addToListModel(model, 0);
        this->usersSource_.addToListModel(model, 0);
        return;
    }

    // Otherwise, make sure to only add maxCount elements in total. We prioritize
    // accepting results from the emote source before the users source (arbitrarily).

    int startingSize = model.rowCount();

    // Add up to maxCount elements
    this->emoteSource_.addToListModel(model, maxCount);

    int used = model.rowCount() - startingSize;
    if (used >= maxCount)
    {
        // Used up our limit on emotes
        return;
    }

    // Only add maxCount - used to ensure the total added doesn't exceed maxCount
    this->usersSource_.addToListModel(model, maxCount - used);
}

void AutocompleteUnifiedSource::addToStringList(QStringList &list,
                                                size_t maxCount,
                                                bool isFirstWord) const
{
    if (maxCount == 0)
    {
        this->emoteSource_.addToStringList(list, 0, isFirstWord);
        this->usersSource_.addToStringList(list, 0, isFirstWord);
        return;
    }

    // Otherwise, make sure to only add maxCount elements in total. We prioritize
    // accepting results from the emote source before the users source (arbitrarily).

    int startingSize = list.size();

    // Add up to maxCount elements
    this->emoteSource_.addToStringList(list, maxCount, isFirstWord);

    int used = list.size() - startingSize;
    if (used >= maxCount)
    {
        // Used up our limit on emotes
        return;
    }

    // Only add maxCount - used to ensure the total added doesn't exceed maxCount
    this->usersSource_.addToStringList(list, maxCount - used, isFirstWord);
}

}  // namespace chatterino
