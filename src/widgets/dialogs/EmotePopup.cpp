#include "EmotePopup.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "widgets/Notebook.hpp"

#include <QHBoxLayout>
#include <QShortcut>
#include <QTabWidget>

namespace chatterino {

EmotePopup::EmotePopup()
    : BaseWindow(nullptr, BaseWindow::EnableCustomFrame)
{
    this->viewEmotes = new ChannelView();
    this->viewEmojis = new ChannelView();

    this->viewEmotes->setOverrideFlags(MessageElement::Flags(
        MessageElement::Default | MessageElement::AlwaysShow | MessageElement::EmoteImages));
    this->viewEmojis->setOverrideFlags(MessageElement::Flags(
        MessageElement::Default | MessageElement::AlwaysShow | MessageElement::EmoteImages));

    this->viewEmotes->setEnableScrollingToBottom(false);
    this->viewEmojis->setEnableScrollingToBottom(false);

    auto *layout = new QVBoxLayout(this);
    this->getLayoutContainer()->setLayout(layout);

    Notebook *notebook = new Notebook(this);
    layout->addWidget(notebook);
    layout->setMargin(0);

    notebook->addPage(this->viewEmotes, "Emotes");
    notebook->addPage(this->viewEmojis, "Emojis");

    this->loadEmojis();

    this->viewEmotes->linkClicked.connect(
        [this](const Link &link) { this->linkClicked.invoke(link); });
    this->viewEmojis->linkClicked.connect(
        [this](const Link &link) { this->linkClicked.invoke(link); });
}

void EmotePopup::loadChannel(ChannelPtr _channel)
{
    this->setWindowTitle("Emotes from " + _channel->name);

    TwitchChannel *channel = dynamic_cast<TwitchChannel *>(_channel.get());

    if (channel == nullptr) {
        return;
    }

    ChannelPtr emoteChannel(new Channel("", Channel::None));

    auto addEmotes = [&](util::EmoteMap &map, const QString &title, const QString &emoteDesc) {
        // TITLE
        messages::MessageBuilder builder1;

        builder1.append(new TextElement(title, MessageElement::Text));

        builder1.getMessage()->flags |= Message::Centered;
        emoteChannel->addMessage(builder1.getMessage());

        // EMOTES
        messages::MessageBuilder builder2;
        builder2.getMessage()->flags |= Message::Centered;
        builder2.getMessage()->flags |= Message::DisableCompactEmotes;

        map.each([&](const QString &key, const util::EmoteData &value) {
            builder2.append((new EmoteElement(value, MessageElement::Flags::AlwaysShow))
                                ->setLink(Link(Link::InsertText, key)));
        });

        emoteChannel->addMessage(builder2.getMessage());
    };

    auto app = getApp();

    QString userID = app->accounts->twitch.getCurrent()->getUserId();

    // fourtf: the entire emote manager needs to be refactored so there's no point in trying to
    // fix this pile of garbage
    for (const auto &set : app->emotes->twitch.emotes[userID].emoteSets) {
        // TITLE
        messages::MessageBuilder builder1;

        QString setText;
        if (set->text.isEmpty()) {
            if (set->channelName.isEmpty()) {
                setText = "Twitch Account Emotes";
            } else {
                setText = "Twitch Account Emotes (" + set->channelName + ")";
            }
        } else {
            setText = set->text;
        }

        builder1.append(new TextElement(setText, MessageElement::Text));

        builder1.getMessage()->flags |= Message::Centered;
        emoteChannel->addMessage(builder1.getMessage());

        // EMOTES
        messages::MessageBuilder builder2;
        builder2.getMessage()->flags |= Message::Centered;
        builder2.getMessage()->flags |= Message::DisableCompactEmotes;

        for (const auto &emote : set->emotes) {
            [&](const QString &key, const util::EmoteData &value) {
                builder2.append((new EmoteElement(value, MessageElement::Flags::AlwaysShow))
                                    ->setLink(Link(Link::InsertText, key)));
            }(emote.code, app->emotes->twitch.getEmoteById(emote.id, emote.code));
        }

        emoteChannel->addMessage(builder2.getMessage());
    }

    addEmotes(app->emotes->bttv.globalEmotes, "BetterTTV Global Emotes", "BetterTTV Global Emote");
    addEmotes(*channel->bttvChannelEmotes.get(), "BetterTTV Channel Emotes",
              "BetterTTV Channel Emote");
    addEmotes(app->emotes->ffz.globalEmotes, "FrankerFaceZ Global Emotes",
              "FrankerFaceZ Global Emote");
    addEmotes(*channel->ffzChannelEmotes.get(), "FrankerFaceZ Channel Emotes",
              "FrankerFaceZ Channel Emote");

    this->viewEmotes->setChannel(emoteChannel);
}

void EmotePopup::loadEmojis()
{
    auto &emojis = getApp()->emotes->emojis.emojis;

    ChannelPtr emojiChannel(new Channel("", Channel::None));

    // title
    messages::MessageBuilder builder1;

    builder1.append(new TextElement("emojis", MessageElement::Text));
    builder1.getMessage()->flags |= Message::Centered;
    emojiChannel->addMessage(builder1.getMessage());

    // emojis
    messages::MessageBuilder builder;
    builder.getMessage()->flags |= Message::Centered;
    builder.getMessage()->flags |= Message::DisableCompactEmotes;

    emojis.each([&builder](const QString &key, const auto &value) {
        builder.append(
            (new EmoteElement(value->emoteData, MessageElement::Flags::AlwaysShow))
                ->setLink(Link(Link::Type::InsertText, ":" + value->shortCodes[0] + ":")));
    });
    emojiChannel->addMessage(builder.getMessage());

    this->viewEmojis->setChannel(emojiChannel);
}

}  // namespace chatterino
