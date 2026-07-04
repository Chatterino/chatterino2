// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/EmotePopup.hpp"

#include "Application.hpp"
#include "common/enums/MessageContext.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/emotes/EmoteController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/emoji/Emojis.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/QStringHash.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/TrimRegExpValidator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Scrollbar.hpp"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTabWidget>

#include <utility>

namespace {

using namespace chatterino;
using namespace Qt::Literals;

bool emojiHasShortCode(const EmojiPtr &emoji, const QString &shortCode)
{
    auto it = std::ranges::find(emoji->shortCodes, shortCode);
    return it != emoji->shortCodes.end();
}

std::optional<EmotePtr> findEmoteByName(const EmoteName &name,
                                        const EmoteMap &emoteMap)
{
    auto it = emoteMap.find(name);
    return it == emoteMap.cend() ? std::nullopt
                                 : std::optional<EmotePtr>(it->second);
}

QString toEmojiShortCode(const QString &shortCodeWithColons)
{
    if (shortCodeWithColons.length() > 2)
    {
        return shortCodeWithColons.mid(1, shortCodeWithColons.length() - 2);
    }

    return shortCodeWithColons;
}

bool isFavouriteEmoteOrEmoji(const QString &identifier, bool isEmoji)
{
    if (isEmoji)
    {
        const auto &shortCodes = getSettings()->favouriteEmojis.getValue();
        auto shortCode = toEmojiShortCode(identifier);

        auto it = std::ranges::find_if(
            shortCodes, [&shortCode](const auto &otherShortCode) {
                return shortCode == otherShortCode;
            });
        return it != shortCodes.end();
    }

    const auto &emoteNames = getSettings()->favouriteEmotes.getValue();
    auto it = std::ranges::find_if(emoteNames,
                                   [identifier](const auto &otherEmoteName) {
                                       return identifier == otherEmoteName;
                                   });
    return it != emoteNames.end();
}

auto saveFavouriteEmojis(const std::unordered_map<QString, EmojiPtr> &emojis)
{
    QStringList emojiNames;
    emojiNames.reserve(static_cast<qsizetype>(emojis.size()));

    std::ranges::transform(emojis, std::back_inserter(emojiNames),
                           [](const auto &it) {
                               return it.first;
                           });

    getSettings()->favouriteEmojis = emojiNames;
}

auto makeTitleMessage(const QString &title)
{
    MessageBuilder builder;
    builder.emplace<TextElement>(title, MessageElementFlag::Text);
    builder->flags.set(MessageFlag::Centered);
    return builder.release();
}

auto makeEmoteMessageSorted(const std::vector<EmotePtr> &emotes,
                            const QString &emptyText = {})
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

    if (emotes.empty())
    {
        builder.emplace<TextElement>(emptyText, MessageElementFlag::Text,
                                     MessageColor::System);
        return builder.release();
    }

    for (const auto &emote : emotes)
    {
        builder
            .emplace<EmoteElement>(
                emote, MessageElementFlags{MessageElementFlag::AlwaysShow,
                                           MessageElementFlag::Emote})
            ->setLink(Link(Link::InsertText, emote->name.string));
    }

    return builder.release();
}

auto makeEmoteMessage(std::vector<EmotePtr> emotes, const QString &emptyText)
{
    std::sort(emotes.begin(), emotes.end(), [](const auto &l, const auto &r) {
        return compareEmoteStrings(l->name.string, r->name.string);
    });

    return makeEmoteMessageSorted(emotes, emptyText);
}

auto makeEmoteMessage(const EmoteMap &map)
{
    std::vector<EmotePtr> vec;
    vec.reserve(map.size());
    for (const auto &[_name, ptr] : map)
    {
        vec.emplace_back(ptr);
    }
    return makeEmoteMessage(std::move(vec), "No emotes available");
}

auto makeEmojiMessage(const std::vector<EmojiPtr> &emojiMap,
                      const QString &emptyText = {})
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

    if (emojiMap.empty() && !emptyText.isEmpty())
    {
        builder.emplace<TextElement>(emptyText, MessageElementFlag::Text,
                                     MessageColor::System);
        return builder.release();
    }

    for (const auto &value : emojiMap)
    {
        builder
            .emplace<EmoteElement>(
                value->emote,
                MessageElementFlags{MessageElementFlag::AlwaysShow,
                                    MessageElementFlag::EmojiAll})
            ->setLink(
                Link(Link::Type::InsertText, ":" + value->shortCodes[0] + ":"));
    }

    return builder.release();
}

auto makeUnavailableEmoteMessage(const std::vector<QString> &emoteNames)
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);

    for (const auto &emoteName : emoteNames)
    {
        builder
            .emplace<TextElement>(
                emoteName, MessageElementFlags{MessageElementFlag::EmoteText,
                                               MessageElementFlag::AlwaysShow})
            ->setLink(Link(Link::Type::InsertText, emoteName));
    }

    return builder.release();
}

auto makeInfoTextMessage(const QString &text)
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder.emplace<TextElement>(
        text,
        MessageElementFlags{MessageElementFlag::Text,
                            MessageElementFlag::AlwaysShow},
        MessageColor::System);

    return builder.release();
}

void addEmotes(Channel &channel, auto &&emotes, const QString &title)
{
    channel.addMessage(makeTitleMessage(title), MessageContext::Original);
    channel.addMessage(makeEmoteMessage(std::forward<decltype(emotes)>(emotes)),
                       MessageContext::Original);
}

void addTwitchEmoteSets(const std::shared_ptr<const EmoteMap> &local,
                        const std::shared_ptr<const TwitchEmoteSetMap> &sets,
                        Channel &globalChannel, Channel &subChannel,
                        const QString &currentChannelID,
                        const QString &channelName)
{
    if (!local->empty())
    {
        addEmotes(subChannel, *local, channelName % u" (Follower)");
    }

    std::vector<
        std::pair<QString, std::reference_wrapper<const TwitchEmoteSet>>>
        sortedSets;
    sortedSets.reserve(sets->size());
    for (const auto &[_id, set] : *sets)
    {
        if (set.owner->id == currentChannelID)
        {
            // Put current channel emotes at the top
            addEmotes(subChannel, set.emotes, set.title());
        }
        else
        {
            sortedSets.emplace_back(set.title(), std::cref(set));
        }
    }

    std::ranges::sort(sortedSets, [](const auto &a, const auto &b) {
        return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });

    for (const auto &[title, set] : sortedSets)
    {
        addEmotes(set.get().isSubLike ? subChannel : globalChannel,
                  set.get().emotes, title);
    }
}

void loadEmojis(ChannelView &view, const std::vector<EmojiPtr> &emojiMap)
{
    static auto emoteCategoryMap = [&] {
        std::map<QString, std::vector<EmojiPtr>> emoteCatMap;

        for (const auto &emoji : emojiMap)
        {
            auto cat = emoteCatMap.find(emoji->category);
            if (cat != emoteCatMap.end())
            {
                auto &vec = cat->second;
                vec.push_back(emoji);
            }
            else
            {
                emoteCatMap.emplace(emoji->category,
                                    std::vector<EmojiPtr>{emoji});
            }
        }
        return emoteCatMap;
    }();

    ChannelPtr emojiChannel(new Channel("", Channel::Type::None));
    // set the channel first to make sure the scrollbar is at the top
    view.setChannel(emojiChannel);

    for (auto &it : emoteCategoryMap)
    {
        // Skip the Component category for now.
        if (it.first == "Component")
        {
            continue;
        }

        emojiChannel->addMessage(makeTitleMessage(it.first),
                                 MessageContext::Original);
        emojiChannel->addMessage(makeEmojiMessage(it.second),
                                 MessageContext::Original);
    }

    // Add the Component category at the bottom of the picker.
    emojiChannel->addMessage(makeTitleMessage("Component"),
                             MessageContext::Original);
    emojiChannel->addMessage(makeEmojiMessage(emoteCategoryMap["Component"]),
                             MessageContext::Original);
}

void loadEmojis(Channel &channel, const std::vector<EmojiPtr> &emojiMap,
                const QString &title)
{
    channel.addMessage(makeTitleMessage(title), MessageContext::Original);
    channel.addMessage(makeEmojiMessage(emojiMap), MessageContext::Original);
}

// Create an emote
EmoteMap filterEmoteMap(const QString &text, const EmoteMap &emotes)
{
    EmoteMap filteredMap;

    for (const auto &emote : emotes)
    {
        if (emote.first.string.contains(text, Qt::CaseInsensitive))
        {
            filteredMap.insert(emote);
        }
    }

    return filteredMap;
}

}  // namespace

namespace chatterino {

EmotePopup::EmotePopup(QWidget *parent)
    : BasePopup({BaseWindow::EnableCustomFrame, BaseWindow::DisableLayoutSave},
                parent)
    , search_(new QLineEdit())
    , notebook_(new Notebook(this))
{
    // this->setStayInScreenRect(true);
    auto bounds = getApp()->getWindows()->emotePopupBounds();
    if (bounds.size().isEmpty())
    {
        bounds.setSize(QSize{300, 500} * this->scale());
    }
    this->setInitialBounds(bounds, widgets::BoundsChecking::DesiredPosition);

    auto *layout = new QVBoxLayout();
    this->getLayoutContainer()->setLayout(layout);

    QRegularExpression searchRegex("\\S*");
    searchRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *layout2 = new QHBoxLayout();
    layout2->setContentsMargins(8, 8, 8, 8);
    layout2->setSpacing(8);

    this->search_->setPlaceholderText("Search all emotes...");
    this->search_->setValidator(new TrimRegExpValidator(searchRegex));
    this->search_->setClearButtonEnabled(true);
    this->search_->findChild<QAbstractButton *>()->setIcon(
        QPixmap(":/buttons/clearSearch.png"));
    this->search_->installEventFilter(this);
    layout2->addWidget(this->search_);

    layout->addLayout(layout2);

    QObject::connect(this->search_, &QLineEdit::textChanged, this,
                     &EmotePopup::filterEmotes);

    auto clicked = [this](const MessageLayoutElement *hoveredElement,
                          Qt::KeyboardModifiers modifiers) {
        if (modifiers.testFlag(Qt::KeyboardModifier::ControlModifier))
        {
            if (hoveredElement == nullptr)
            {
                return;
            }

            const auto *page = this->notebook_->getSelectedPage();

            auto identifier = hoveredElement->getLink().value;
            if (identifier.isEmpty())
            {
                return;
            }

            if (this->favouritesView_ == page)
            {
                auto isEmoji = hoveredElement->getCreator().getFlags().hasAny(
                    MessageElementFlag::EmojiAll);

                if (isEmoji)
                {
                    this->removeFavouriteEmoji(toEmojiShortCode(identifier));
                }
                else
                {
                    this->removeFavouriteEmote(EmoteName{identifier});
                }
            }
            else if (this->viewEmojis_ == page)
            {
                this->addFavouriteEmoji(toEmojiShortCode(identifier));
            }
            else
            {
                this->addFavouriteEmote(EmoteName{identifier});
            }

            if (!modifiers.testFlag(Qt::KeyboardModifier::ShiftModifier))
            {
                return;
            }
        }

        this->linkClicked.invoke(hoveredElement->getLink());
    };

    auto makeView = [&](QString tabTitle, bool addToNotebook = true) {
        auto *view = new ChannelView(nullptr);

        view->setOverrideFlags(MessageElementFlags{
            MessageElementFlag::Default, MessageElementFlag::AlwaysShow,
            MessageElementFlag::EmoteImage});
        view->setEnableScrollingToBottom(false);
        // We can safely ignore this signal connection since the ChannelView is deleted
        // either when the notebook is deleted, or when our main layout is deleted.
        std::ignore = view->elementClicked.connect(clicked);

        if (addToNotebook)
        {
            this->notebook_->addPage(view, std::move(tabTitle));
        }

        std::ignore = view->messageMenuCreated.connect(
            [this](QMenu *menu, const MessageLayoutElement *hoveredElement) {
                if (hoveredElement == nullptr)
                {
                    return;
                }

                auto flags = hoveredElement->getCreator().getFlags();

                if (!flags.hasAny(MessageElementFlag::EmojiAll,
                                  MessageElementFlag::Emote))
                {
                    return;
                }

                QAction *favouriteAction;
                if (menu->actions().isEmpty())
                {
                    favouriteAction = menu->addAction("Favourite");
                }
                else
                {
                    favouriteAction = new QAction("Favourite");
                    menu->insertAction(menu->actions().constFirst(),
                                       favouriteAction);
                }

                auto isEmoji = flags.hasAny(MessageElementFlag::EmojiAll);
                const auto &identifier = hoveredElement->getLink().value;

                favouriteAction->setCheckable(true);
                favouriteAction->setChecked(
                    isFavouriteEmoteOrEmoji(identifier, isEmoji));

                QObject::connect(
                    favouriteAction, &QAction::triggered,
                    [this, identifier, isEmoji](bool checked) {
                        if (!checked)
                        {
                            if (isEmoji)
                            {
                                this->removeFavouriteEmoji(
                                    toEmojiShortCode(identifier));
                            }
                            else
                            {
                                this->removeFavouriteEmote(
                                    EmoteName{identifier});
                            }
                        }
                        else
                        {
                            if (isEmoji)
                            {
                                this->addFavouriteEmoji(
                                    toEmojiShortCode(identifier));
                            }
                            else
                            {
                                this->addFavouriteEmote(EmoteName{identifier});
                            }
                        }
                    });
            });

        return view;
    };

    this->searchView_ = makeView("", false);
    this->searchView_->hide();
    layout->addWidget(this->searchView_);

    layout->addWidget(this->notebook_);
    layout->setContentsMargins(0, 0, 0, 0);

    this->favouritesView_ = makeView("Favourite");
    this->subEmotesView_ = makeView("Subs");
    this->channelEmotesView_ = makeView("Channel");
    this->globalEmotesView_ = makeView("Global");
    this->viewEmojis_ = makeView("Emojis");

    this->notebook_->select(this->subEmotesView_);

    loadEmojis(*this->viewEmojis_,
               getApp()->getEmotes()->getEmojis()->getEmojis());
    this->addShortcuts();
    this->signalHolder_.managedConnect(getApp()->getHotkeys()->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });

    this->search_->setFocus();

    this->signalHolder_.managedConnect(
        getApp()->getAccounts()->twitch.emotesReloaded,
        [this](auto * /*caller*/, const auto &result) {
            if (!result)
            {
                // some error occurred, no need to reload
                return;
            }
            this->reloadEmotes();
        });

    this->themeChangedEvent();
}

void EmotePopup::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"openTab",  // CTRL + 1-8 to open corresponding tab.
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.empty())
             {
                 qCWarning(chatterinoHotkeys)
                     << "openTab shortcut called without arguments. Takes "
                        "only one argument: tab specifier";
                 return "openTab shortcut called without arguments. "
                        "Takes only one argument: tab specifier";
             }
             auto target = arguments.at(0);
             if (target == "last")
             {
                 this->notebook_->selectLastTab();
             }
             else if (target == "next")
             {
                 this->notebook_->selectNextTab();
             }
             else if (target == "previous")
             {
                 this->notebook_->selectPreviousTab();
             }
             else
             {
                 bool ok{false};
                 int result = target.toInt(&ok);
                 if (ok)
                 {
                     this->notebook_->selectVisibleIndex(result, false);
                 }
                 else
                 {
                     qCWarning(chatterinoHotkeys)
                         << "Invalid argument for openTab shortcut";
                     return QString("Invalid argument for openTab "
                                    "shortcut: \"%1\". Use \"last\", "
                                    "\"next\", \"previous\" or an integer.")
                         .arg(target);
                 }
             }
             return "";
         }},
        {"delete",
         [this](const std::vector<QString> &) -> QString {
             this->close();
             return "";
         }},
        {"scrollPage",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.empty())
             {
                 qCWarning(chatterinoHotkeys)
                     << "scrollPage hotkey called without arguments!";
                 return "scrollPage hotkey called without arguments!";
             }
             auto direction = arguments.at(0);
             auto *channelView = dynamic_cast<ChannelView *>(
                 this->notebook_->getSelectedPage());

             auto &scrollbar = channelView->getScrollBar();
             if (direction == "up")
             {
                 scrollbar.offset(-scrollbar.getPageSize());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getPageSize());
             }
             else
             {
                 qCWarning(chatterinoHotkeys) << "Unknown scroll direction";
             }
             return "";
         }},

        {"reject", nullptr},
        {"accept", nullptr},
        {"search",
         [this](const std::vector<QString> &) -> QString {
             this->search_->setFocus();
             this->search_->selectAll();
             return "";
         }},
    };

    this->shortcuts_ = getApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);
}

void EmotePopup::loadChannel(ChannelPtr channel)
{
    BenchmarkGuard guard("loadChannel");

    this->channel_ = std::move(channel);
    this->twitchChannel_ = dynamic_cast<TwitchChannel *>(this->channel_.get());

    this->setWindowTitle("Emotes in #" + this->channel_->getName());

    this->globalEmotesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->subEmotesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->channelEmotesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->searchView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->favouritesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));

    this->reloadEmotes();
}

void EmotePopup::addFavouriteEmoji(const QString &shortCode)
{
    if (shortCode.isEmpty())
    {
        return;
    }
    if (this->favouriteEmojis_.contains(shortCode))
    {
        return;
    }

    for (const auto &emoji : getApp()->getEmotes()->getEmojis()->getEmojis())
    {
        if (emojiHasShortCode(emoji, shortCode))
        {
            this->favouriteEmojis_.emplace(shortCode, emoji);
            break;
        }
    }

    this->updateFavouriteEmotesAndEmojis();
    saveFavouriteEmojis(this->favouriteEmojis_);
}

void EmotePopup::addFavouriteEmote(const EmoteName &name)
{
    //
    // Note that we are checking the persistent list of favourite emote names
    // rather than the internal vector of favouriteEmotes_. We do this because
    // in order to populate the favouriteEmotes_ list, we first need to download
    // all Emotes we have access to from Twitch. This can take considerable
    // time during which the persistent list and the internal list are
    // effectively out of sync. If there is a connection issue, these two lists
    // may not sync up at all. Using the persistent list is the safe choice.
    //
    auto emoteNames = getSettings()->favouriteEmotes.getValue();
    for (const auto &emotePresentName : emoteNames)
    {
        if (emotePresentName == name.string)
        {
            return;
        }
    }
    auto emote = this->findEmote(name);
    if (!emote)
    {
        return;
    }

    this->favouriteEmotes_.push_back(std::move(*emote));

    emoteNames.push_back(name.string);
    getSettings()->favouriteEmotes = emoteNames;

    this->updateFavouriteEmotesAndEmojis();
}

void EmotePopup::removeFavouriteEmoji(const QString &shortCode)
{
    this->favouriteEmojis_.erase(shortCode);
    saveFavouriteEmojis(this->favouriteEmojis_);

    this->updateFavouriteEmotesAndEmojis();
}

void EmotePopup::removeFavouriteEmote(const EmoteName &name)
{
    std::erase_if(this->favouriteEmotes_, [name](const auto &emote) {
        return emote->name == name;
    });

    auto emoteNames = getSettings()->favouriteEmotes.getValue();
    emoteNames.removeIf([name](const auto &emoteName) {
        return emoteName == name.string;
    });
    getSettings()->favouriteEmotes = emoteNames;

    this->updateFavouriteEmotesAndEmojis();
}

void EmotePopup::updateFavouriteEmotesAndEmojis()
{
    auto chan = this->favouritesView_->underlyingChannel();
    chan->clearMessages();

    if (this->favouriteEmotes_.empty() && this->favouriteEmojis_.empty())
    {
        auto msg = makeInfoTextMessage(
            "No favourites. You can add them by Ctrl+clicking on an Emote or "
            "marking it as favourite in the context menu");
        chan->addMessage(msg, MessageContext::Original);

        return;
    }

    // Add Emotes
    if (!this->favouriteEmotes_.empty())
    {
        chan->addMessage(makeEmoteMessageSorted(this->favouriteEmotes_),
                         MessageContext::Original);
    }

    // Add Emojis
    if (!this->favouriteEmojis_.empty())
    {
        std::vector<EmojiPtr> emojis;
        emojis.reserve(this->favouriteEmotes_.size());
        std::ranges::transform(this->favouriteEmojis_,
                               std::back_inserter(emojis), [](const auto &v) {
                                   return v.second;
                               });
        chan->addMessage(makeEmojiMessage(emojis), MessageContext::Original);
    }

    // Show favourited Emotes that are currently not available
    std::vector<QString> unavailableEmotes;
    for (const auto &emoteName : getSettings()->favouriteEmotes.getValue())
    {
        auto it = std::ranges::find_if(
            this->favouriteEmotes_, [emoteName](const auto &emote) {
                return emoteName == emote->name.string;
            });
        if (it == this->favouriteEmotes_.end())
        {
            unavailableEmotes.push_back(emoteName);
        }
    }
    if (!unavailableEmotes.empty())
    {
        static const auto explainUnavailability =
            u"Emotes can be unavailable because they are specific for a "
            u"particular channel, you are no longer subscribed to a channel "
            u"that provides the emotes or we were unable to verify that you "
            u"have access to an emote due to network issues."_s;

        auto msg =
            makeInfoTextMessage("Currently unavailable favourite emotes");
        chan->addMessage(msg, MessageContext::Original);

        msg = makeInfoTextMessage(explainUnavailability);
        chan->addMessage(msg, MessageContext::Original);

        chan->addMessage(makeUnavailableEmoteMessage(unavailableEmotes),
                         MessageContext::Original);
    }
}

void EmotePopup::reloadEmotes()
{
    auto subChannel = this->subEmotesView_->underlyingChannel();
    auto globalChannel = this->globalEmotesView_->underlyingChannel();
    auto channelChannel = this->channelEmotesView_->underlyingChannel();

    subChannel->clearMessages();
    globalChannel->clearMessages();
    channelChannel->clearMessages();

    if (this->twitchChannel_)
    {
        // twitch
        addTwitchEmoteSets(
            twitchChannel_->localTwitchEmotes(),
            *getApp()->getAccounts()->twitch.getCurrent()->accessEmoteSets(),
            *globalChannel, *subChannel, twitchChannel_->roomId(),
            twitchChannel_->getName());

        // channel
        if (getSettings()->enableBTTVChannelEmotes)
        {
            addEmotes(*channelChannel, *this->twitchChannel_->bttvEmotes(),
                      "BetterTTV");
        }
        if (getSettings()->enableFFZChannelEmotes)
        {
            addEmotes(*channelChannel, *this->twitchChannel_->ffzEmotes(),
                      "FrankerFaceZ");
        }
        if (getSettings()->enableSevenTVChannelEmotes)
        {
            addEmotes(*channelChannel, *this->twitchChannel_->seventvEmotes(),
                      "7TV");
        }
    }
    // global
    if (getSettings()->enableBTTVGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->getBttvEmotes()->emotes(),
                  "BetterTTV");
    }
    if (getSettings()->enableFFZGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->getFfzEmotes()->emotes(),
                  "FrankerFaceZ");
    }
    if (getSettings()->enableSevenTVGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->getSeventvEmotes()->globalEmotes(),
                  "7TV");
    }

    this->favouriteEmotes_.clear();
    const auto &emoteNames = getSettings()->favouriteEmotes;
    for (const auto &emoteName : emoteNames.getValue())
    {
        auto emote = this->findEmote(EmoteName{emoteName});
        if (emote)
        {
            this->favouriteEmotes_.push_back(*emote);
        }
    }
    this->favouriteEmojis_.clear();
    const auto &emojiShortCodes = getSettings()->favouriteEmojis;
    for (const auto &shortCode : emojiShortCodes.getValue())
    {
        for (const auto &emoji :
             getApp()->getEmotes()->getEmojis()->getEmojis())
        {
            if (emojiHasShortCode(emoji, shortCode))
            {
                this->favouriteEmojis_.emplace(shortCode, emoji);
                break;
            }
        }
    }
    this->updateFavouriteEmotesAndEmojis();

    if (!subChannel->hasMessages())
    {
        MessageBuilder builder;
        builder->flags.set(MessageFlag::Centered);
        builder->flags.set(MessageFlag::DisableCompactEmotes);
        builder.emplace<TextElement>("no subscription emotes available",
                                     MessageElementFlag::Text,
                                     MessageColor::System);
        subChannel->addMessage(builder.release(), MessageContext::Original);
    }
}

bool EmotePopup::eventFilter(QObject *object, QEvent *event)
{
    if (object == this->search_ && event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (keyEvent == QKeySequence::DeleteStartOfWord &&
            this->search_->selectionLength() > 0)
        {
            this->search_->backspace();
            return true;
        }
    }
    return false;
}

void EmotePopup::filterTwitchEmotes(std::shared_ptr<Channel> searchChannel,
                                    const QString &searchText)
{
    if (this->twitchChannel_)
    {
        auto local = filterEmoteMap(searchText,
                                    *this->twitchChannel_->localTwitchEmotes());
        if (!local.empty())
        {
            addEmotes(*searchChannel, local,
                      this->twitchChannel_->getName() % u" (Follower)");
        }

        for (const auto &[_id, set] :
             **getApp()->getAccounts()->twitch.getCurrent()->accessEmoteSets())
        {
            auto filtered = filterEmoteMap(searchText, set.emotes);
            if (!filtered.empty())
            {
                addEmotes(*searchChannel, std::move(filtered), set.title());
            }
        }
    }

    auto bttvGlobalEmotes =
        filterEmoteMap(searchText, *getApp()->getBttvEmotes()->emotes());
    auto ffzGlobalEmotes =
        filterEmoteMap(searchText, *getApp()->getFfzEmotes()->emotes());
    auto seventvGlobalEmotes = filterEmoteMap(
        searchText, *getApp()->getSeventvEmotes()->globalEmotes());

    // global
    if (!bttvGlobalEmotes.empty())
    {
        addEmotes(*searchChannel, bttvGlobalEmotes, "BetterTTV (Global)");
    }
    if (!ffzGlobalEmotes.empty())
    {
        addEmotes(*searchChannel, ffzGlobalEmotes, "FrankerFaceZ (Global)");
    }
    if (!seventvGlobalEmotes.empty())
    {
        addEmotes(*searchChannel, seventvGlobalEmotes, "7TV (Global)");
    }

    if (this->twitchChannel_ == nullptr)
    {
        return;
    }

    auto bttvChannelEmotes =
        filterEmoteMap(searchText, *this->twitchChannel_->bttvEmotes());
    auto ffzChannelEmotes =
        filterEmoteMap(searchText, *this->twitchChannel_->ffzEmotes());
    auto seventvChannelEmotes =
        filterEmoteMap(searchText, *this->twitchChannel_->seventvEmotes());

    // channel
    if (!bttvChannelEmotes.empty())
    {
        addEmotes(*searchChannel, bttvChannelEmotes, "BetterTTV (Channel)");
    }
    if (!ffzChannelEmotes.empty())
    {
        addEmotes(*searchChannel, ffzChannelEmotes, "FrankerFaceZ (Channel)");
    }
    if (!seventvChannelEmotes.empty())
    {
        addEmotes(*searchChannel, seventvChannelEmotes, "7TV (Channel)");
    }
}

void EmotePopup::filterEmotes(const QString &searchText)
{
    if (searchText.length() == 0)
    {
        this->notebook_->show();
        this->searchView_->hide();

        return;
    }
    auto searchChannel = this->searchView_->underlyingChannel();
    searchChannel->clearMessages();

    // true in special channels like /mentions
    if (this->channel_->isTwitchChannel())
    {
        this->filterTwitchEmotes(searchChannel, searchText);
    }

    std::vector<EmojiPtr> filteredEmojis{};
    int emojiCount = 0;

    const auto &emojis = getApp()->getEmotes()->getEmojis()->getEmojis();
    for (const auto &emoji : emojis)
    {
        if (emoji->shortCodes[0].contains(searchText, Qt::CaseInsensitive))
        {
            filteredEmojis.push_back(emoji);
            emojiCount++;
        }
    }
    // emojis
    if (emojiCount > 0)
    {
        loadEmojis(*searchChannel, filteredEmojis, "Emojis");
    }

    this->notebook_->hide();
    this->searchView_->show();
}

std::optional<EmotePtr> EmotePopup::findEmote(const EmoteName &name)
{
    if (this->twitchChannel_)
    {
        auto emotesToTry = this->twitchChannel_->localTwitchEmotes();

        auto emote = findEmoteByName(name, *emotesToTry);
        if (emote)
        {
            return emote;
        }

        auto twitchEmotes =
            *getApp()->getAccounts()->twitch.getCurrent()->accessEmoteSets();
        //
        // Check the Emote set for the currently active channel first.
        // If multiple channels share an Emote name, it probably makes sense
        // to use the Emote for the channel that is active - the channel we
        // would send the Emote to. This gives the user a chance to see what
        // Emote would the other chatters see in their own chats.
        //
        auto currentChannelID = this->twitchChannel_->roomId();
        auto currentChannelIt = std::ranges::find_if(
            *twitchEmotes, [currentChannelID](const auto &it) {
                return it.second.owner->id == currentChannelID;
            });
        if (currentChannelIt != twitchEmotes->end())
        {
            const auto &emoteSet = currentChannelIt->second;
            auto emote = findEmoteByName(name, emoteSet.emotes);
            if (emote)
            {
                return emote;
            }
        }

        for (const auto &[setId, emoteSet] : *twitchEmotes)
        {
            auto emote = findEmoteByName(name, emoteSet.emotes);
            if (emote)
            {
                return emote;
            }
        }

        emote = this->twitchChannel_->ffzEmote(name);
        if (emote)
        {
            return emote;
        }

        emote = this->twitchChannel_->bttvEmote(name);
        if (emote)
        {
            return emote;
        }

        emote = this->twitchChannel_->seventvEmote(name);
        if (emote)
        {
            return emote;
        }
    }

    auto emote = getApp()->getFfzEmotes()->emote(name);
    if (emote)
    {
        return emote;
    }

    emote = getApp()->getBttvEmotes()->emote(name);
    if (emote)
    {
        return emote;
    }

    emote = getApp()->getSeventvEmotes()->globalEmote(name);
    if (emote)
    {
        return emote;
    }

    return std::nullopt;
}

void EmotePopup::saveBounds() const
{
    if (isAppAboutToQuit())
    {
        return;
    }

    auto bounds = this->getBounds();
    if (!bounds.isNull())
    {
        getApp()->getWindows()->setEmotePopupBounds(bounds);
    }
}

void EmotePopup::resizeEvent(QResizeEvent *event)
{
    this->saveBounds();
    BasePopup::resizeEvent(event);
}

void EmotePopup::moveEvent(QMoveEvent *event)
{
    this->saveBounds();
    BasePopup::moveEvent(event);
}

void EmotePopup::themeChangedEvent()
{
    BasePopup::themeChangedEvent();

    this->setPalette(getTheme()->palette);
}

void EmotePopup::closeEvent(QCloseEvent *event)
{
    this->saveBounds();
    BasePopup::closeEvent(event);
}

}  // namespace chatterino
