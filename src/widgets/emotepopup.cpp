#include "emotepopup.hpp"

#include <QHBoxLayout>
#include <QTabWidget>

#include "messages/messagebuilder.hpp"
#include "twitch/twitchchannel.hpp"

using namespace chatterino::twitch;
using namespace chatterino::messages;

namespace chatterino {
namespace widgets {

EmotePopup::EmotePopup(singletons::ThemeManager &themeManager)
    : BaseWidget(themeManager, 0)
{
    this->initAsWindow();

    this->viewEmotes = new ChannelView();
    this->viewEmojis = new ChannelView();

    this->viewEmotes->setEnableScrollingToBottom(false);
    this->viewEmojis->setEnableScrollingToBottom(false);

    this->setLayout(new QVBoxLayout(this));

    QTabWidget *tabs = new QTabWidget(this);
    this->layout()->addWidget(tabs);
    this->layout()->setMargin(0);
    tabs->addTab(this->viewEmotes, "Emotes");
    tabs->addTab(this->viewEmojis, "Emojis");

    this->loadEmojis();
}

void EmotePopup::loadChannel(std::shared_ptr<Channel> _channel)
{
    TwitchChannel *channel = dynamic_cast<TwitchChannel *>(_channel.get());

    if (channel == nullptr) {
        return;
    }

    std::shared_ptr<Channel> emoteChannel(new Channel(""));

    auto addEmotes = [&](util::EmoteMap &map, const QString &title, const QString &emoteDesc) {
        // TITLE
        messages::MessageBuilder builder1;

        builder1.appendWord(Word(title, Word::Flags::Text, MessageColor(MessageColor::Text),
                                 singletons::FontManager::Medium, QString(), QString()));

        builder1.getMessage()->centered = true;
        emoteChannel->addMessage(builder1.getMessage());

        // EMOTES
        messages::MessageBuilder builder2;
        builder2.getMessage()->centered = true;

        map.each([&](const QString &key, const util::EmoteData &value) {
            builder2.appendWord(Word(value.image, Word::Flags::AlwaysShow, key, emoteDesc,
                                     Link(Link::Type::InsertText, key)));
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
    util::EmoteMap &emojis = singletons::EmoteManager::getInstance().getEmojis();

    std::shared_ptr<Channel> emojiChannel(new Channel(""));

    // title
    messages::MessageBuilder builder1;

    builder1.appendWord(Word("emojis", Word::Flags::Text, MessageColor(MessageColor::Text),
                             singletons::FontManager::Medium, QString(), QString()));

    builder1.getMessage()->centered = true;
    emojiChannel->addMessage(builder1.getMessage());

    // emojis
    messages::MessageBuilder builder;
    builder.getMessage()->centered = true;
    emojis.each([this, &builder](const QString &key, const util::EmoteData &value) {
        builder.appendWord(Word(value.image, Word::Flags::AlwaysShow, key, "emoji",
                                Link(Link::Type::InsertText, key)));
    });
    emojiChannel->addMessage(builder.getMessage());

    this->viewEmojis->setChannel(emojiChannel);
}

}  // namespace widgets
}  // namespace chatterino
