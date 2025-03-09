#include "widgets/dialogs/EmotePopup.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
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

auto makeTitleMessage(const QString &title)
{
    MessageBuilder builder;
    builder.emplace<TextElement>(title, MessageElementFlag::Text);
    builder->flags.set(MessageFlag::Centered);
    return builder.release();
}

auto makeEmoteMessage(std::vector<EmotePtr> emotes,
                      const MessageElementFlag &emoteFlag)
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

    if (emotes.empty())
    {
        builder.emplace<TextElement>("no emotes available",
                                     MessageElementFlag::Text,
                                     MessageColor::System);
        return builder.release();
    }

    std::sort(emotes.begin(), emotes.end(), [](const auto &l, const auto &r) {
        return compareEmoteStrings(l->name.string, r->name.string);
    });
    for (const auto &emote : emotes)
    {
        builder
            .emplace<EmoteElement>(
                emote,
                MessageElementFlags{MessageElementFlag::AlwaysShow, emoteFlag})
            ->setLink(Link(Link::InsertText, emote->name.string));
    }

    return builder.release();
}

auto makeEmoteMessage(const EmoteMap &map, const MessageElementFlag &emoteFlag)
{
    if (map.empty())
    {
        MessageBuilder builder;
        builder->flags.set(MessageFlag::Centered);
        builder->flags.set(MessageFlag::DisableCompactEmotes);
        builder.emplace<TextElement>("no emotes available",
                                     MessageElementFlag::Text,
                                     MessageColor::System);
        return builder.release();
    }

    std::vector<EmotePtr> vec;
    vec.reserve(map.size());
    for (const auto &[_name, ptr] : map)
    {
        vec.emplace_back(ptr);
    }
    return makeEmoteMessage(std::move(vec), emoteFlag);
}

auto makeEmojiMessage(const std::vector<EmojiPtr> &emojiMap)
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

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

void addEmotes(Channel &channel, auto &&emotes, const QString &title,
               const MessageElementFlag &emoteFlag)
{
    channel.addMessage(makeTitleMessage(title), MessageContext::Original);
    channel.addMessage(
        makeEmoteMessage(std::forward<decltype(emotes)>(emotes), emoteFlag),
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
        addEmotes(subChannel, *local, channelName % u" (Follower)",
                  MessageElementFlag::TwitchEmote);
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
            addEmotes(subChannel, set.emotes, set.title(),
                      MessageElementFlag::TwitchEmote);
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
                  set.get().emotes, title, MessageElementFlag::TwitchEmote);
    }
}

void loadEmojis(ChannelView &view, const std::vector<EmojiPtr> &emojiMap)
{
    ChannelPtr emojiChannel(new Channel("", Channel::Type::None));
    // set the channel first to make sure the scrollbar is at the top
    view.setChannel(emojiChannel);

    emojiChannel->addMessage(makeEmojiMessage(emojiMap),
                             MessageContext::Original);
}

void loadEmojis(Channel &channel, const std::vector<EmojiPtr> &emojiMap,
                const QString &title)
{
    channel.addMessage(makeTitleMessage(title), MessageContext::Original);
    channel.addMessage(makeEmojiMessage(emojiMap), MessageContext::Original);
}

// Create an emote
EmoteMap filterEmoteMap(const QString &text,
                        const std::shared_ptr<const EmoteMap> &emotes)
{
    EmoteMap filteredMap;

    for (const auto &emote : *emotes)
    {
        if (emote.first.string.contains(text, Qt::CaseInsensitive))
        {
            filteredMap.insert(emote);
        }
    }

    return filteredMap;
}

std::vector<EmotePtr> filterEmoteVec(const QString &text,
                                     const std::vector<EmotePtr> &emotes)
{
    std::vector<EmotePtr> filtered;

    for (const auto &emote : emotes)
    {
        if (emote->name.string.contains(text, Qt::CaseInsensitive))
        {
            filtered.emplace_back(emote);
        }
    }

    return filtered;
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

    auto clicked = [this](const Link &link) {
        this->linkClicked.invoke(link);
    };

    auto makeView = [&](QString tabTitle, bool addToNotebook = true) {
        auto *view = new ChannelView(nullptr);

        view->setOverrideFlags(MessageElementFlags{
            MessageElementFlag::Default, MessageElementFlag::AlwaysShow,
            MessageElementFlag::EmoteImages});
        view->setEnableScrollingToBottom(false);
        // We can safely ignore this signal connection since the ChannelView is deleted
        // either when the notebook is deleted, or when our main layout is deleted.
        std::ignore = view->linkClicked.connect(clicked);

        if (addToNotebook)
        {
            this->notebook_->addPage(view, std::move(tabTitle));
        }

        return view;
    };

    this->searchView_ = makeView("", false);
    this->searchView_->hide();
    layout->addWidget(this->searchView_);

    layout->addWidget(this->notebook_);
    layout->setContentsMargins(0, 0, 0, 0);

    this->subEmotesView_ = makeView("Subs");
    this->channelEmotesView_ = makeView("Channel");
    this->globalEmotesView_ = makeView("Global");
    this->viewEmojis_ = makeView("Emojis");

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

    if (this->twitchChannel_ == nullptr)
    {
        return;
    }

    this->globalEmotesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->subEmotesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->channelEmotesView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));
    this->searchView_->setChannel(
        std::make_shared<Channel>("", Channel::Type::None));

    this->reloadEmotes();
}

void EmotePopup::reloadEmotes()
{
    if (this->twitchChannel_ == nullptr)
    {
        return;
    }

    auto subChannel = this->subEmotesView_->underlyingChannel();
    auto globalChannel = this->globalEmotesView_->underlyingChannel();
    auto channelChannel = this->channelEmotesView_->underlyingChannel();

    subChannel->clearMessages();
    globalChannel->clearMessages();
    channelChannel->clearMessages();

    // twitch
    addTwitchEmoteSets(
        twitchChannel_->localTwitchEmotes(),
        *getApp()->getAccounts()->twitch.getCurrent()->accessEmoteSets(),
        *globalChannel, *subChannel, twitchChannel_->roomId(),
        twitchChannel_->getName());

    // global
    if (Settings::instance().enableBTTVGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->getBttvEmotes()->emotes(),
                  "BetterTTV", MessageElementFlag::BttvEmote);
    }
    if (Settings::instance().enableFFZGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->getFfzEmotes()->emotes(),
                  "FrankerFaceZ", MessageElementFlag::FfzEmote);
    }
    if (Settings::instance().enableSevenTVGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->getSeventvEmotes()->globalEmotes(),
                  "7TV", MessageElementFlag::SevenTVEmote);
    }

    // channel
    if (Settings::instance().enableBTTVChannelEmotes)
    {
        addEmotes(*channelChannel, *this->twitchChannel_->bttvEmotes(),
                  "BetterTTV", MessageElementFlag::BttvEmote);
    }
    if (Settings::instance().enableFFZChannelEmotes)
    {
        addEmotes(*channelChannel, *this->twitchChannel_->ffzEmotes(),
                  "FrankerFaceZ", MessageElementFlag::FfzEmote);
    }
    if (Settings::instance().enableSevenTVChannelEmotes)
    {
        addEmotes(*channelChannel, *this->twitchChannel_->seventvEmotes(),
                  "7TV", MessageElementFlag::SevenTVEmote);
    }

    if (subChannel->getMessageSnapshot().size() == 0)
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
                                    this->twitchChannel_->localTwitchEmotes());
        if (!local.empty())
        {
            addEmotes(*searchChannel, local,
                      this->twitchChannel_->getName() % u" (Follower)",
                      MessageElementFlag::TwitchEmote);
        }

        for (const auto &[_id, set] :
             **getApp()->getAccounts()->twitch.getCurrent()->accessEmoteSets())
        {
            auto filtered = filterEmoteVec(searchText, set.emotes);
            if (!filtered.empty())
            {
                addEmotes(*searchChannel, std::move(filtered), set.title(),
                          MessageElementFlag::TwitchEmote);
            }
        }
    }

    auto bttvGlobalEmotes =
        filterEmoteMap(searchText, getApp()->getBttvEmotes()->emotes());
    auto ffzGlobalEmotes =
        filterEmoteMap(searchText, getApp()->getFfzEmotes()->emotes());
    auto seventvGlobalEmotes = filterEmoteMap(
        searchText, getApp()->getSeventvEmotes()->globalEmotes());

    // global
    if (!bttvGlobalEmotes.empty())
    {
        addEmotes(*searchChannel, bttvGlobalEmotes, "BetterTTV (Global)",
                  MessageElementFlag::BttvEmote);
    }
    if (!ffzGlobalEmotes.empty())
    {
        addEmotes(*searchChannel, ffzGlobalEmotes, "FrankerFaceZ (Global)",
                  MessageElementFlag::FfzEmote);
    }
    if (!seventvGlobalEmotes.empty())
    {
        addEmotes(*searchChannel, seventvGlobalEmotes, "7TV (Global)",
                  MessageElementFlag::SevenTVEmote);
    }

    if (this->twitchChannel_ == nullptr)
    {
        return;
    }

    auto bttvChannelEmotes =
        filterEmoteMap(searchText, this->twitchChannel_->bttvEmotes());
    auto ffzChannelEmotes =
        filterEmoteMap(searchText, this->twitchChannel_->ffzEmotes());
    auto seventvChannelEmotes =
        filterEmoteMap(searchText, this->twitchChannel_->seventvEmotes());

    // channel
    if (!bttvChannelEmotes.empty())
    {
        addEmotes(*searchChannel, bttvChannelEmotes, "BetterTTV (Channel)",
                  MessageElementFlag::BttvEmote);
    }
    if (!ffzChannelEmotes.empty())
    {
        addEmotes(*searchChannel, ffzChannelEmotes, "FrankerFaceZ (Channel)",
                  MessageElementFlag::FfzEmote);
    }
    if (!seventvChannelEmotes.empty())
    {
        addEmotes(*searchChannel, seventvChannelEmotes, "7TV (Channel)",
                  MessageElementFlag::SevenTVEmote);
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

void EmotePopup::saveBounds() const
{
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

    // NOTE: This currently overrides the QLineEdit's font size.
    // If the dialog is open when the theme is changed, things will look a bit off.
    // This can be alleviated by us using a single application-wide style sheet for these things.
    this->search_->setStyleSheet(this->theme->splits.input.styleSheet);
}

void EmotePopup::closeEvent(QCloseEvent *event)
{
    this->saveBounds();
    BasePopup::closeEvent(event);
}

}  // namespace chatterino
