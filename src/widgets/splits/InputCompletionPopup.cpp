#include "InputCompletionPopup.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Emote.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/listview/GenericListView.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

namespace chatterino {
namespace {

    struct _Emote {
        EmotePtr emote;
        QString displayName;
        QString providerName;
    };

    void addEmotes(std::vector<_Emote> &out, const EmoteMap &map,
                   const QString &text, const QString &providerName)
    {
        for (auto &&emote : map)
            if (emote.first.string.contains(text, Qt::CaseInsensitive))
                out.push_back(
                    {emote.second, emote.second->name.string, providerName});
    }

    void addEmojis(std::vector<_Emote> &out, const EmojiMap &map,
                   const QString &text)
    {
        map.each([&](const QString &, const std::shared_ptr<EmojiData> &emoji) {
            for (auto &&shortCode : emoji->shortCodes)
                if (shortCode.contains(text, Qt::CaseInsensitive))
                    out.push_back({emoji->emote, shortCode, "Emoji"});
        });
    }
}  // namespace

InputCompletionPopup::InputCompletionPopup(QWidget *parent)
    : BasePopup({BasePopup::EnableCustomFrame, BasePopup::Frameless,
                 BasePopup::DontFocus},
                parent)
    , model_(this)
{
    this->initLayout();

    QObject::connect(&this->redrawTimer_, &QTimer::timeout, this, [this] {
        if (this->isVisible())
            this->ui_.listView->doItemsLayout();
    });
    this->redrawTimer_.setInterval(33);
}

void InputCompletionPopup::initLayout()
{
    LayoutCreator creator = {this};

    auto listView =
        creator.emplace<GenericListView>().assign(&this->ui_.listView);
    listView->setInvokeActionOnTab(true);

    listView->setModel(&this->model_);
    QObject::connect(listView.getElement(), &GenericListView::closeRequested,
                     this, [this] {
                         this->close();
                     });
}

void InputCompletionPopup::updateEmotes(const QString &text, ChannelPtr channel)
{
    std::vector<_Emote> emotes;
    auto tc = dynamic_cast<TwitchChannel *>(channel.get());
    // returns true also for special Twitch channels (/live, /mentions, /whispers, etc.)
    if (channel->isTwitchChannel())
    {
        if (auto user = getApp()->accounts->twitch.getCurrent())
        {
            // Twitch Emotes available globally
            auto emoteData = user->accessEmotes();
            addEmotes(emotes, emoteData->emotes, text, "Twitch Emote");

            // Twitch Emotes available locally
            auto localEmoteData = user->accessLocalEmotes();
            if (tc &&
                localEmoteData->find(tc->roomId()) != localEmoteData->end())
            {
                if (auto localEmotes = &localEmoteData->at(tc->roomId()))
                {
                    addEmotes(emotes, *localEmotes, text,
                              "Local Twitch Emotes");
                }
            }
        }

        if (tc)
        {
            // TODO extract "Channel {BetterTTV,7TV,FrankerFaceZ}" text into a #define.
            if (auto bttv = tc->bttvEmotes())
                addEmotes(emotes, *bttv, text, "Channel BetterTTV");
            if (auto ffz = tc->ffzEmotes())
                addEmotes(emotes, *ffz, text, "Channel FrankerFaceZ");
            if (auto seventv = tc->seventvEmotes())
            {
                addEmotes(emotes, *seventv, text, "Channel 7TV");
            }
        }

        if (auto bttvG = getApp()->twitch->getBttvEmotes().emotes())
            addEmotes(emotes, *bttvG, text, "Global BetterTTV");
        if (auto ffzG = getApp()->twitch->getFfzEmotes().emotes())
            addEmotes(emotes, *ffzG, text, "Global FrankerFaceZ");
        if (auto seventvG = getApp()->twitch->getSeventvEmotes().globalEmotes())
        {
            addEmotes(emotes, *seventvG, text, "Global 7TV");
        }
    }

    addEmojis(emotes, getApp()->emotes->emojis.emojis, text);

    // if there is an exact match, put that emote first
    for (size_t i = 1; i < emotes.size(); i++)
    {
        auto emoteText = emotes.at(i).displayName;

        // test for match or match with colon at start for emotes like ":)"
        if (emoteText.compare(text, Qt::CaseInsensitive) == 0 ||
            emoteText.compare(":" + text, Qt::CaseInsensitive) == 0)
        {
            auto emote = emotes[i];
            emotes.erase(emotes.begin() + int(i));
            emotes.insert(emotes.begin(), emote);
            break;
        }
    }

    this->model_.clear();

    int count = 0;
    for (auto &&emote : emotes)
    {
        this->model_.addItem(std::make_unique<InputCompletionItem>(
            emote.emote, emote.displayName + " - " + emote.providerName,
            this->callback_));

        if (count++ == maxEntryCount)
            break;
    }

    if (!emotes.empty())
    {
        this->ui_.listView->setCurrentIndex(this->model_.index(0));
    }
}

void InputCompletionPopup::updateUsers(const QString &text, ChannelPtr channel)
{
    auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel)
    {
        auto chatters = twitchChannel->accessChatters()->filterByPrefix(text);
        this->model_.clear();
        int count = 0;
        for (const auto &name : chatters)
        {
            this->model_.addItem(std::make_unique<InputCompletionItem>(
                nullptr, name, this->callback_));

            if (count++ == maxEntryCount)
                break;
        }
        if (!chatters.empty())
        {
            this->ui_.listView->setCurrentIndex(this->model_.index(0));
        }
    }
}

bool InputCompletionPopup::eventFilter(QObject *watched, QEvent *event)
{
    return this->ui_.listView->eventFilter(watched, event);
}

void InputCompletionPopup::setInputAction(ActionCallback callback)
{
    this->callback_ = std::move(callback);
}

void InputCompletionPopup::showEvent(QShowEvent *)
{
    this->redrawTimer_.start();
}

void InputCompletionPopup::hideEvent(QHideEvent *)
{
    this->redrawTimer_.stop();
}

}  // namespace chatterino
