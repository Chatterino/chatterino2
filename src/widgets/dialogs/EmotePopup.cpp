#include "EmotePopup.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "debug/Benchmark.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "widgets/Notebook.hpp"

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

    for (const auto &emote : map) {
        builder
            .emplace<EmoteElement>(emote.second, MessageElementFlag::AlwaysShow)
            ->setLink(Link(Link::InsertText, emote.first.string));
    }

    return builder.release();
}
void addEmoteSets(std::vector<std::shared_ptr<TwitchAccount::EmoteSet>> sets,
                  Channel &globalChannel, Channel &subChannel)
{
    for (const auto &set : sets) {
        auto &channel = set->key == "0" ? globalChannel : subChannel;

        // TITLE
        auto text =
            set->key == "0" || set->text.isEmpty() ? "Twitch" : set->text;
        channel.addMessage(makeTitleMessage(text));

        // EMOTES
        MessageBuilder builder;
        builder->flags.set(MessageFlag::Centered);
        builder->flags.set(MessageFlag::DisableCompactEmotes);

        for (const auto &emote : set->emotes) {
            builder
                .emplace<EmoteElement>(
                    getApp()->emotes->twitch.getOrCreateEmote(emote.id,
                                                              emote.name),
                    MessageElementFlag::AlwaysShow)
                ->setLink(Link(Link::InsertText, emote.name.string));
        }

        channel.addMessage(builder.release());
    }
}
}  // namespace

EmotePopup::EmotePopup()
    : BaseWindow(nullptr, BaseWindow::EnableCustomFrame)
{
    auto layout = new QVBoxLayout(this);
    this->getLayoutContainer()->setLayout(layout);

    auto notebook = new Notebook(this);
    layout->addWidget(notebook);
    layout->setMargin(0);

    auto clicked = [this](const Link &link) { this->linkClicked.invoke(link); };

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
}

void EmotePopup::loadChannel(ChannelPtr _channel)
{
    BenchmarkGuard guard("loadChannel");

    this->setWindowTitle("Emotes from " + _channel->getName());

    auto twitchChannel = dynamic_cast<TwitchChannel *>(_channel.get());
    if (twitchChannel == nullptr) return;

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
        *globalChannel, *subChannel);

    // global
    addEmotes(*globalChannel, *getApp()->emotes->bttv.global(), "BetterTTV");
    addEmotes(*globalChannel, *getApp()->emotes->ffz.global(), "FrankerFaceZ");

    // channel
    addEmotes(*channelChannel, *twitchChannel->bttvEmotes(), "BetterTTV");
    addEmotes(*channelChannel, *twitchChannel->ffzEmotes(), "FrankerFaceZ");

    this->globalEmotesView_->setChannel(globalChannel);
    this->subEmotesView_->setChannel(subChannel);
    this->channelEmotesView_->setChannel(channelChannel);
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

}  // namespace chatterino
