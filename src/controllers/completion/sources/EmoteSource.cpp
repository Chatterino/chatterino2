#include "controllers/completion/sources/EmoteSource.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/completion/sources/Helpers.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"

namespace chatterino::completion {

namespace {

    void addEmotes(std::vector<EmoteItem> &out, const EmoteMap &map,
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

    void addEmojis(std::vector<EmoteItem> &out,
                   const std::vector<EmojiPtr> &map)
    {
        for (const auto &emoji : map)
        {
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
        };
    }

}  // namespace

EmoteSource::EmoteSource(const Channel *channel,
                         std::unique_ptr<EmoteStrategy> strategy,
                         ActionCallback callback)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
{
    this->initializeFromChannel(channel);
}

void EmoteSource::update(const QString &query)
{
    this->output_.clear();
    if (this->strategy_)
    {
        this->strategy_->apply(this->items_, this->output_, query);
    }
}

void EmoteSource::addToListModel(GenericListModel &model, size_t maxCount) const
{
    addVecToListModel(this->output_, model, maxCount,
                      [this](const EmoteItem &e) {
                          return std::make_unique<InputCompletionItem>(
                              e.emote, e.displayName + " - " + e.providerName,
                              this->callback_);
                      });
}

void EmoteSource::addToStringList(QStringList &list, size_t maxCount,
                                  bool /* isFirstWord */) const
{
    addVecToStringList(this->output_, list, maxCount, [](const EmoteItem &e) {
        return e.tabCompletionName + " ";
    });
}

void EmoteSource::initializeFromChannel(const Channel *channel)
{
    auto *app = getApp();

    std::vector<EmoteItem> emotes;
    const auto *tc = dynamic_cast<const TwitchChannel *>(channel);
    // returns true also for special Twitch channels (/live, /mentions, /whispers, etc.)
    if (channel->isTwitchChannel())
    {
        if (tc)
        {
            if (auto twitch = tc->localTwitchEmotes())
            {
                addEmotes(emotes, *twitch, "Local Twitch Emotes");
            }

            auto user = getApp()->getAccounts()->twitch.getCurrent();
            addEmotes(emotes, **user->accessEmotes(), "Twitch Emote");

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

        if (auto bttvG = app->getBttvEmotes()->emotes())
        {
            addEmotes(emotes, *bttvG, "Global BetterTTV");
        }
        if (auto ffzG = app->getFfzEmotes()->emotes())
        {
            addEmotes(emotes, *ffzG, "Global FrankerFaceZ");
        }
        if (auto seventvG = app->getSeventvEmotes()->globalEmotes())
        {
            addEmotes(emotes, *seventvG, "Global 7TV");
        }
    }

    addEmojis(emotes, app->getEmotes()->getEmojis()->getEmojis());

    this->items_ = std::move(emotes);
}

const std::vector<EmoteItem> &EmoteSource::output() const
{
    return this->output_;
}

}  // namespace chatterino::completion
