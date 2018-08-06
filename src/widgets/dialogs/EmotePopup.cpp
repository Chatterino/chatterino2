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
    this->viewEmotes_ = new ChannelView();
    this->viewEmojis_ = new ChannelView();

    this->viewEmotes_->setOverrideFlags(MessageElement::Flags(
        MessageElement::Default | MessageElement::AlwaysShow |
        MessageElement::EmoteImages));
    this->viewEmojis_->setOverrideFlags(MessageElement::Flags(
        MessageElement::Default | MessageElement::AlwaysShow |
        MessageElement::EmoteImages));

    this->viewEmotes_->setEnableScrollingToBottom(false);
    this->viewEmojis_->setEnableScrollingToBottom(false);

    auto *layout = new QVBoxLayout(this);
    this->getLayoutContainer()->setLayout(layout);

    Notebook *notebook = new Notebook(this);
    layout->addWidget(notebook);
    layout->setMargin(0);

    notebook->addPage(this->viewEmotes_, "Emotes");
    notebook->addPage(this->viewEmojis_, "Emojis");

    this->loadEmojis();

    this->viewEmotes_->linkClicked.connect(
        [this](const Link &link) { this->linkClicked.invoke(link); });
    this->viewEmojis_->linkClicked.connect(
        [this](const Link &link) { this->linkClicked.invoke(link); });
}

void EmotePopup::loadChannel(ChannelPtr _channel)
{
    this->setWindowTitle("Emotes from " + _channel->getName());

    TwitchChannel *channel = dynamic_cast<TwitchChannel *>(_channel.get());

    if (channel == nullptr) {
        return;
    }

    ChannelPtr emoteChannel(new Channel("", Channel::Type::None));

    auto addEmotes = [&](const EmoteMap &map, const QString &title,
                         const QString &emoteDesc) {
        // TITLE
        MessageBuilder builder1;

        builder1.append(new TextElement(title, MessageElement::Text));

        builder1.getMessage()->flags |= Message::Centered;
        emoteChannel->addMessage(builder1.getMessage());

        // EMOTES
        MessageBuilder builder2;
        builder2.getMessage()->flags |= Message::Centered;
        builder2.getMessage()->flags |= Message::DisableCompactEmotes;

        for (auto emote : map) {
            builder2.append(
                (new EmoteElement(emote.second,
                                  MessageElement::Flags::AlwaysShow))
                    ->setLink(Link(Link::InsertText, emote.first.string)));
        }

        emoteChannel->addMessage(builder2.getMessage());
    };

    auto app = getApp();

    // fourtf: the entire emote manager needs to be refactored so there's no
    // point in trying to fix this pile of garbage
    for (const auto &set :
         app->accounts->twitch.getCurrent()->accessEmotes()->emoteSets) {
        // TITLE
        MessageBuilder builder1;

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
        MessageBuilder builder2;
        builder2.getMessage()->flags |= Message::Centered;
        builder2.getMessage()->flags |= Message::DisableCompactEmotes;

        for (const auto &emote : set->emotes) {
            builder2.append(
                (new EmoteElement(
                     app->emotes->twitch.getOrCreateEmote(emote.id, emote.name),
                     MessageElement::Flags::AlwaysShow))
                    ->setLink(Link(Link::InsertText, emote.name.string)));
        }

        emoteChannel->addMessage(builder2.getMessage());
    }

    addEmotes(*app->emotes->bttv.accessGlobalEmotes(),
              "BetterTTV Global Emotes", "BetterTTV Global Emote");
    addEmotes(*channel->accessBttvEmotes(), "BetterTTV Channel Emotes",
              "BetterTTV Channel Emote");
    //    addEmotes(*app->emotes->ffz.accessGlobalEmotes(), "FrankerFaceZ Global
    //    Emotes",
    //              "FrankerFaceZ Global Emote");
    addEmotes(*channel->accessFfzEmotes(), "FrankerFaceZ Channel Emotes",
              "FrankerFaceZ Channel Emote");

    this->viewEmotes_->setChannel(emoteChannel);
}

void EmotePopup::loadEmojis()
{
    auto &emojis = getApp()->emotes->emojis.emojis;

    ChannelPtr emojiChannel(new Channel("", Channel::Type::None));

    // title
    MessageBuilder builder1;

    builder1.append(new TextElement("emojis", MessageElement::Text));
    builder1.getMessage()->flags |= Message::Centered;
    emojiChannel->addMessage(builder1.getMessage());

    // emojis
    MessageBuilder builder;
    builder.getMessage()->flags |= Message::Centered;
    builder.getMessage()->flags |= Message::DisableCompactEmotes;

    emojis.each([&builder](const auto &key, const auto &value) {
        builder.append(
            (new EmoteElement(value->emote, MessageElement::Flags::AlwaysShow))
                ->setLink(Link(Link::Type::InsertText,
                               ":" + value->shortCodes[0] + ":")));
    });
    emojiChannel->addMessage(builder.getMessage());

    this->viewEmojis_->setChannel(emojiChannel);
}

}  // namespace chatterino
