#include "providers/autocomplete/AutocompleteEmoteSource.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"

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

}  // namespace

AutocompleteEmoteSource::AutocompleteEmoteSource(
    const Channel *channel, ActionCallback callback,
    std::unique_ptr<AutocompleteEmoteStrategy> strategy)
    : AutocompleteGenericSource({}, std::move(strategy))  // begin with no items
    , callback_(std::move(callback))
{
    this->initializeItems(channel);
}

void AutocompleteEmoteSource::initializeItems(const Channel *channel)
{
    auto app = getIApp();

    std::vector<CompletionEmote> emotes;
    auto *tc = dynamic_cast<const TwitchChannel *>(channel);
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

    this->setItems(std::move(emotes));
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

}  // namespace chatterino
