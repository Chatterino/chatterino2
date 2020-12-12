#include "EmotePopup.hpp"

#include "Application.hpp"
#include "common/CompletionModel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Shortcut.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QHBoxLayout>
#include <QShortcut>
#include <QTabWidget>

namespace chatterino {
namespace {
    auto makeTitleMessage(const QString &title)
    {
        MessageBuilder builder;
        builder.emplace<TextElement>(title, MessageElementFlag::Text);
        builder->flags.set(MessageFlag::Centered);
        return builder.release();
    }
    auto makeEmoteMessage(const EmoteMap &map)
    {
        MessageBuilder builder;
        builder->flags.set(MessageFlag::Centered);
        builder->flags.set(MessageFlag::DisableCompactEmotes);

        if (!map.empty())
        {
            std::vector<std::pair<EmoteName, EmotePtr>> vec(map.begin(),
                                                            map.end());
            std::sort(vec.begin(), vec.end(),
                      [](const std::pair<EmoteName, EmotePtr> &l,
                         const std::pair<EmoteName, EmotePtr> &r) {
                          return CompletionModel::compareStrings(
                              l.first.string, r.first.string);
                      });
            for (const auto &emote : vec)
            {
                builder
                    .emplace<EmoteElement>(emote.second,
                                           MessageElementFlag::AlwaysShow)
                    ->setLink(Link(Link::InsertText, emote.first.string));
            }
        }
        else
        {
            builder.emplace<TextElement>("no emotes available",
                                         MessageElementFlag::Text,
                                         MessageColor::System);
        }

        return builder.release();
    }
    void addEmoteSets(
        std::vector<std::shared_ptr<TwitchAccount::EmoteSet>> sets,
        Channel &globalChannel, Channel &subChannel, QString currentChannelName)
    {
        QMap<QString, QPair<bool, std::vector<MessagePtr>>> mapOfSets;

        for (const auto &set : sets)
        {
            // TITLE
            auto channelName = set->channelName;
            auto text =
                set->key == "0" || set->text.isEmpty() ? "Twitch" : set->text;

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
                        MessageElementFlag::AlwaysShow)
                    ->setLink(Link(Link::InsertText, emote.name.string));
            }

            mapOfSets[channelName].second.push_back(builder.release());
        }

        // Output to channel all created messages,
        // That contain title or emotes.
        // Put current channel emotes at the top
        auto currentChannelPair = mapOfSets[currentChannelName];
        for (auto message : currentChannelPair.second)
        {
            subChannel.addMessage(message);
        }
        mapOfSets.remove(currentChannelName);

        foreach (auto pair, mapOfSets)
        {
            auto &channel = pair.first ? globalChannel : subChannel;
            for (auto message : pair.second)
            {
                channel.addMessage(message);
            }
        }
    }
}  // namespace

EmotePopup::EmotePopup(QWidget *parent)
    : BasePopup(BaseWindow::EnableCustomFrame, parent)
{
    this->setStayInScreenRect(true);
    this->moveTo(this, getApp()->windows->emotePopupPos(), false);

    auto layout = new QVBoxLayout(this);
    this->getLayoutContainer()->setLayout(layout);

    auto notebook = new Notebook(this);
    layout->addWidget(notebook);
    layout->setMargin(0);

    auto clicked = [this](const Link &link) {
        this->linkClicked.invoke(link);
    };

    auto makeView = [&](QString tabTitle) {
        auto view = new ChannelView();

        view->setOverrideFlags(MessageElementFlags{
            MessageElementFlag::Default, MessageElementFlag::AlwaysShow,
            MessageElementFlag::EmoteImages});
        view->setEnableScrollingToBottom(false);
        notebook->addPage(view, tabTitle);
        view->linkClicked.connect(clicked);

        return view;
    };

    this->subEmotesView_ = makeView("Subs");
    this->channelEmotesView_ = makeView("Channel");
    this->globalEmotesView_ = makeView("Global");
    this->viewEmojis_ = makeView("Emojis");

    this->loadEmojis();

    // CTRL + 1-8 to open corresponding tab
    for (auto i = 0; i < 8; i++)
    {
        const auto openTab = [this, i, notebook] {
            notebook->selectIndex(i);
        };
        createWindowShortcut(this, QString("CTRL+%1").arg(i + 1).toUtf8(),
                             openTab);
    }

    // Open last tab (first one from right)
    createWindowShortcut(this, "CTRL+9", [=] {
        notebook->selectLastTab();
    });

    // Cycle through tabs
    createWindowShortcut(this, "CTRL+Tab", [=] {
        notebook->selectNextTab();
    });
    createWindowShortcut(this, "CTRL+Shift+Tab", [=] {
        notebook->selectPreviousTab();
    });
}

void EmotePopup::loadChannel(ChannelPtr _channel)
{
    BenchmarkGuard guard("loadChannel");

    this->setWindowTitle("Emotes in #" + _channel->getName());

    auto twitchChannel = dynamic_cast<TwitchChannel *>(_channel.get());
    if (twitchChannel == nullptr)
        return;

    auto addEmotes = [&](Channel &channel, const EmoteMap &map,
                         const QString &title) {
        channel.addMessage(makeTitleMessage(title));
        channel.addMessage(makeEmoteMessage(map));
    };

    auto subChannel = std::make_shared<Channel>("", Channel::Type::None);
    auto globalChannel = std::make_shared<Channel>("", Channel::Type::None);
    auto channelChannel = std::make_shared<Channel>("", Channel::Type::None);

    // twitch
    addEmoteSets(
        getApp()->accounts->twitch.getCurrent()->accessEmotes()->emoteSets,
        *globalChannel, *subChannel, _channel->getName());

    // global
    addEmotes(*globalChannel, *twitchChannel->globalBttv().emotes(),
              "BetterTTV");
    addEmotes(*globalChannel, *twitchChannel->globalFfz().emotes(),
              "FrankerFaceZ");

    // channel
    addEmotes(*channelChannel, *twitchChannel->bttvEmotes(), "BetterTTV");
    addEmotes(*channelChannel, *twitchChannel->ffzEmotes(), "FrankerFaceZ");

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

void EmotePopup::loadEmojis()
{
    auto &emojis = getApp()->emotes->emojis.emojis;

    ChannelPtr emojiChannel(new Channel("", Channel::Type::None));

    // emojis
    MessageBuilder builder;
    builder->flags.set(MessageFlag::Centered);
    builder->flags.set(MessageFlag::DisableCompactEmotes);

    emojis.each([&builder](const auto &key, const auto &value) {
        builder
            .emplace<EmoteElement>(value->emote, MessageElementFlag::AlwaysShow)
            ->setLink(
                Link(Link::Type::InsertText, ":" + value->shortCodes[0] + ":"));
    });
    emojiChannel->addMessage(builder.release());

    this->viewEmojis_->setChannel(emojiChannel);
}

void EmotePopup::closeEvent(QCloseEvent *event)
{
    getApp()->windows->setEmotePopupPos(this->pos());
    QWidget::closeEvent(event);
}
}  // namespace chatterino
