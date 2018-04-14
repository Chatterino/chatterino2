#include "emotepopup.hpp"

#include <QHBoxLayout>
#include <QTabWidget>

#include "messages/messagebuilder.hpp"
#include "providers/twitch/twitchchannel.hpp"

using namespace chatterino::providers::twitch;
using namespace chatterino::messages;

namespace chatterino {
namespace widgets {

EmotePopup::EmotePopup(singletons::ThemeManager &themeManager)
    : BaseWindow(themeManager, 0)
{
    this->viewEmotes = new ChannelView();
    this->viewEmojis = new ChannelView();

    this->viewEmotes->setOverrideFlags((MessageElement::Flags)(
        MessageElement::Default | MessageElement::AlwaysShow | MessageElement::EmoteImages));
    this->viewEmojis->setOverrideFlags((MessageElement::Flags)(
        MessageElement::Default | MessageElement::AlwaysShow | MessageElement::EmoteImages));

    this->viewEmotes->setEnableScrollingToBottom(false);
    this->viewEmojis->setEnableScrollingToBottom(false);

    this->setLayout(new QVBoxLayout(this));

    QTabWidget *tabs = new QTabWidget(this);
    this->layout()->addWidget(tabs);
    this->layout()->setMargin(0);
    tabs->addTab(this->viewEmotes, "Emotes");
    tabs->addTab(this->viewEmojis, "Emojis");

    this->loadEmojis();

    this->viewEmotes->linkClicked.connect(
        [this](const Link &link) { this->linkClicked.invoke(link); });
    this->viewEmojis->linkClicked.connect(
        [this](const Link &link) { this->linkClicked.invoke(link); });
}

void EmotePopup::loadChannel(ChannelPtr _channel)
{
    TwitchChannel *channel = dynamic_cast<TwitchChannel *>(_channel.get());

    if (channel == nullptr) {
        return;
    }

    ChannelPtr emoteChannel(new Channel(""));

    auto addEmotes = [&](util::EmoteMap &map, const QString &title, const QString &emoteDesc) {
        // TITLE
        messages::MessageBuilder builder1;

        builder1.append(new TextElement(title, MessageElement::Text));

        builder1.getMessage()->flags &= Message::Centered;
        emoteChannel->addMessage(builder1.getMessage());

        // EMOTES
        messages::MessageBuilder builder2;
        builder2.getMessage()->flags &= Message::Centered;
        builder2.getMessage()->flags &= Message::DisableCompactEmotes;

        map.each([&](const QString &key, const util::EmoteData &value) {
            builder2.append((new EmoteElement(value, MessageElement::Flags::AlwaysShow))
                                ->setLink(Link(Link::InsertText, key)));
        });

        emoteChannel->addMessage(builder2.getMessage());
    };

    singletons::EmoteManager &emoteManager = singletons::EmoteManager::getInstance();

    addEmotes(emoteManager.bttvGlobalEmotes, "BetterTTV Global Emotes", "BetterTTV Global Emote");
    addEmotes(*channel->bttvChannelEmotes.get(), "BetterTTV Channel Emotes",
              "BetterTTV Channel Emote");
    addEmotes(emoteManager.ffzGlobalEmotes, "FrankerFaceZ Global Emotes",
              "FrankerFaceZ Global Emote");
    addEmotes(*channel->ffzChannelEmotes.get(), "FrankerFaceZ Channel Emotes",
              "FrankerFaceZ Channel Emote");

    this->viewEmotes->setChannel(emoteChannel);
}

void EmotePopup::loadEmojis()
{
    auto &emojis = singletons::EmoteManager::getInstance().getEmojis();

    ChannelPtr emojiChannel(new Channel(""));

    // title
    messages::MessageBuilder builder1;

    builder1.append(new TextElement("emojis", MessageElement::Text));
    builder1.getMessage()->flags &= Message::Centered;
    emojiChannel->addMessage(builder1.getMessage());

    // emojis
    messages::MessageBuilder builder;
    builder.getMessage()->flags &= Message::Centered;
    builder.getMessage()->flags &= Message::DisableCompactEmotes;

    emojis.each([&builder](const QString &key, const auto &value) {
        builder.append((new EmoteElement(value.emoteData, MessageElement::Flags::AlwaysShow))
                           ->setLink(Link(Link::Type::InsertText, ":" + value.shortCode + ":")));
    });
    emojiChannel->addMessage(builder.getMessage());

    this->viewEmojis->setChannel(emojiChannel);
}

}  // namespace widgets
}  // namespace chatterino
