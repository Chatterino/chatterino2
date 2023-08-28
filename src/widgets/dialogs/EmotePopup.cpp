#include "EmotePopup.hpp"

#include "Application.hpp"
#include "common/CompletionModel.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/TrimRegExpValidator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Scrollbar.hpp"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QRegularExpression>
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

auto makeEmoteMessage(const EmoteMap &map, const MessageElementFlag &emoteFlag)
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

    if (map.empty())
    {
        builder.emplace<TextElement>("no emotes available",
                                     MessageElementFlag::Text,
                                     MessageColor::System);
        return builder.release();
    }

    std::vector<std::pair<EmoteName, EmotePtr>> vec(map.begin(), map.end());
    std::sort(vec.begin(), vec.end(),
              [](const std::pair<EmoteName, EmotePtr> &l,
                 const std::pair<EmoteName, EmotePtr> &r) {
                  return CompletionModel::compareStrings(l.first.string,
                                                         r.first.string);
              });
    for (const auto &emote : vec)
    {
        builder
            .emplace<EmoteElement>(
                emote.second,
                MessageElementFlags{MessageElementFlag::AlwaysShow, emoteFlag})
            ->setLink(Link(Link::InsertText, emote.first.string));
    }

    return builder.release();
}

auto makeEmojiMessage(EmojiMap &emojiMap)
{
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

    emojiMap.each([&builder](const auto &key, const auto &value) {
        (void)key;  // unused

        builder
            .emplace<EmoteElement>(
                value->emote,
                MessageElementFlags{MessageElementFlag::AlwaysShow,
                                    MessageElementFlag::EmojiAll})
            ->setLink(
                Link(Link::Type::InsertText, ":" + value->shortCodes[0] + ":"));
    });

    return builder.release();
}

void addTwitchEmoteSets(
    std::vector<std::shared_ptr<TwitchAccount::EmoteSet>> sets,
    Channel &globalChannel, Channel &subChannel, QString currentChannelName)
{
    QMap<QString, QPair<bool, std::vector<MessagePtr>>> mapOfSets;

    for (const auto &set : sets)
    {
        // Some emotes (e.g. follower ones) are only available in their origin channel
        if (set->local && currentChannelName != set->channelName)
        {
            continue;
        }

        // TITLE
        auto channelName = set->channelName;
        auto text = set->text.isEmpty() ? "Twitch" : set->text;

        // EMOTES
        MessageBuilder builder;
        builder->flags.set(MessageFlag::Centered);
        builder->flags.set(MessageFlag::DisableCompactEmotes);

        // If value of map is empty, create init pair and add title.
        if (mapOfSets.find(channelName) == mapOfSets.end())
        {
            std::vector<MessagePtr> b;
            b.push_back(makeTitleMessage(text));
            mapOfSets[channelName] = qMakePair(set->key == "0", b);
        }

        for (const auto &emote : set->emotes)
        {
            builder
                .emplace<EmoteElement>(
                    getApp()->emotes->twitch.getOrCreateEmote(emote.id,
                                                              emote.name),
                    MessageElementFlags{MessageElementFlag::AlwaysShow,
                                        MessageElementFlag::TwitchEmote})
                ->setLink(Link(Link::InsertText, emote.name.string));
        }

        mapOfSets[channelName].second.push_back(builder.release());
    }

    // Output to channel all created messages,
    // That contain title or emotes.
    // Put current channel emotes at the top
    auto currentChannelPair = mapOfSets[currentChannelName];
    for (const auto &message : currentChannelPair.second)
    {
        subChannel.addMessage(message);
    }
    mapOfSets.remove(currentChannelName);

    for (const auto &pair : mapOfSets)
    {
        auto &channel = pair.first ? globalChannel : subChannel;
        for (const auto &message : pair.second)
        {
            channel.addMessage(message);
        }
    }
}

void addEmotes(Channel &channel, const EmoteMap &map, const QString &title,
               const MessageElementFlag &emoteFlag)
{
    channel.addMessage(makeTitleMessage(title));
    channel.addMessage(makeEmoteMessage(map, emoteFlag));
}

void loadEmojis(ChannelView &view, EmojiMap &emojiMap)
{
    ChannelPtr emojiChannel(new Channel("", Channel::Type::None));
    emojiChannel->addMessage(makeEmojiMessage(emojiMap));

    view.setChannel(emojiChannel);
}

void loadEmojis(Channel &channel, EmojiMap &emojiMap, const QString &title)
{
    channel.addMessage(makeTitleMessage(title));
    channel.addMessage(makeEmojiMessage(emojiMap));
}

// Create an emote
EmoteMap filterEmoteMap(const QString &text,
                        std::shared_ptr<const EmoteMap> emotes)
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

}  // namespace

namespace chatterino {

EmotePopup::EmotePopup(QWidget *parent)
    : BasePopup(BaseWindow::EnableCustomFrame, parent)
    , search_(new QLineEdit())
    , notebook_(new Notebook(this))
{
    // this->setStayInScreenRect(true);
    this->moveTo(getApp()->windows->emotePopupPos(),
                 BaseWindow::BoundsChecker::DesiredPosition);

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
    layout2->addWidget(this->search_);

    layout->addLayout(layout2);

    QObject::connect(this->search_, &QLineEdit::textChanged, this,
                     &EmotePopup::filterEmotes);

    auto clicked = [this](const Link &link) {
        this->linkClicked.invoke(link);
    };

    auto makeView = [&](QString tabTitle, bool addToNotebook = true) {
        auto *view = new ChannelView();

        view->setOverrideFlags(MessageElementFlags{
            MessageElementFlag::Default, MessageElementFlag::AlwaysShow,
            MessageElementFlag::EmoteImages});
        view->setEnableScrollingToBottom(false);
        view->linkClicked.connect(clicked);

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

    loadEmojis(*this->viewEmojis_, getApp()->emotes->emojis.emojis);
    this->addShortcuts();
    this->signalHolder_.managedConnect(getApp()->hotkeys->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });

    this->search_->setFocus();
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
                 scrollbar.offset(-scrollbar.getLargeChange());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getLargeChange());
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

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
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

    auto subChannel = std::make_shared<Channel>("", Channel::Type::None);
    auto globalChannel = std::make_shared<Channel>("", Channel::Type::None);
    auto channelChannel = std::make_shared<Channel>("", Channel::Type::None);

    // twitch
    addTwitchEmoteSets(
        getApp()->accounts->twitch.getCurrent()->accessEmotes()->emoteSets,
        *globalChannel, *subChannel, this->channel_->getName());

    // global
    if (Settings::instance().enableBTTVGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->twitch->getBttvEmotes().emotes(),
                  "BetterTTV", MessageElementFlag::BttvEmote);
    }
    if (Settings::instance().enableFFZGlobalEmotes)
    {
        addEmotes(*globalChannel, *getApp()->twitch->getFfzEmotes().emotes(),
                  "FrankerFaceZ", MessageElementFlag::FfzEmote);
    }
    if (Settings::instance().enableSevenTVGlobalEmotes)
    {
        addEmotes(*globalChannel,
                  *getApp()->twitch->getSeventvEmotes().globalEmotes(), "7TV",
                  MessageElementFlag::SevenTVEmote);
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

    this->globalEmotesView_->setChannel(globalChannel);
    this->subEmotesView_->setChannel(subChannel);
    this->channelEmotesView_->setChannel(channelChannel);

    if (subChannel->getMessageSnapshot().size() == 0)
    {
        MessageBuilder builder;
        builder->flags.set(MessageFlag::Centered);
        builder->flags.set(MessageFlag::DisableCompactEmotes);
        builder.emplace<TextElement>("no subscription emotes available",
                                     MessageElementFlag::Text,
                                     MessageColor::System);
        subChannel->addMessage(builder.release());
    }
}

void EmotePopup::filterTwitchEmotes(std::shared_ptr<Channel> searchChannel,
                                    const QString &searchText)
{
    auto twitchEmoteSets =
        getApp()->accounts->twitch.getCurrent()->accessEmotes()->emoteSets;
    std::vector<std::shared_ptr<TwitchAccount::EmoteSet>> twitchGlobalEmotes{};

    for (const auto &set : twitchEmoteSets)
    {
        auto setCopy = std::make_shared<TwitchAccount::EmoteSet>(*set);
        auto setIt =
            std::remove_if(setCopy->emotes.begin(), setCopy->emotes.end(),
                           [searchText](auto &emote) {
                               return !emote.name.string.contains(
                                   searchText, Qt::CaseInsensitive);
                           });
        setCopy->emotes.resize(std::distance(setCopy->emotes.begin(), setIt));

        if (!setCopy->emotes.empty())
        {
            twitchGlobalEmotes.push_back(setCopy);
        }
    }

    auto bttvGlobalEmotes =
        filterEmoteMap(searchText, getApp()->twitch->getBttvEmotes().emotes());
    auto ffzGlobalEmotes =
        filterEmoteMap(searchText, getApp()->twitch->getFfzEmotes().emotes());
    auto seventvGlobalEmotes = filterEmoteMap(
        searchText, getApp()->twitch->getSeventvEmotes().globalEmotes());

    // twitch
    addTwitchEmoteSets(twitchGlobalEmotes, *searchChannel, *searchChannel,
                       this->channel_->getName());

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
    auto searchChannel = std::make_shared<Channel>("", Channel::Type::None);

    // true in special channels like /mentions
    if (this->channel_->isTwitchChannel())
    {
        this->filterTwitchEmotes(searchChannel, searchText);
    }

    EmojiMap filteredEmojis{};
    int emojiCount = 0;

    getApp()->emotes->emojis.emojis.each(
        [&, searchText](const auto &name, std::shared_ptr<EmojiData> &emoji) {
            if (emoji->shortCodes[0].contains(searchText, Qt::CaseInsensitive))
            {
                filteredEmojis.insert(name, emoji);
                emojiCount++;
            }
        });
    // emojis
    if (emojiCount > 0)
    {
        loadEmojis(*searchChannel, filteredEmojis, "Emojis");
    }

    this->searchView_->setChannel(searchChannel);

    this->notebook_->hide();
    this->searchView_->show();
}

void EmotePopup::closeEvent(QCloseEvent *event)
{
    getApp()->windows->setEmotePopupPos(this->pos());
    BaseWindow::closeEvent(event);
}

}  // namespace chatterino
