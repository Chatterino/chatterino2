#include "emotepopup.hpp"

#include <QHBoxLayout>

#include "messages/messagebuilder.hpp"
#include "twitch/twitchchannel.hpp"

using namespace chatterino::twitch;
using namespace chatterino::messages;

namespace chatterino {
namespace widgets {

EmotePopup::EmotePopup(ColorScheme &colorScheme, EmoteManager &emoteManager,
                       WindowManager &windowManager)
    : BaseWidget(colorScheme, 0)
    , emoteManager(emoteManager)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    this->setLayout(layout);
    layout->setMargin(0);

    view = new ChannelView(windowManager, this);
    layout->addWidget(view);
}

void EmotePopup::loadChannel(std::shared_ptr<Channel> _channel)
{
    TwitchChannel *channel = dynamic_cast<TwitchChannel *>(_channel.get());

    if (channel == nullptr) {
        return;
    }

    std::shared_ptr<Channel> emoteChannel(new Channel);

    auto addEmotes = [&](EmoteMap &map, const QString &title, const QString &emoteDesc) {
        // TITLE
        messages::MessageBuilder builder1;

        builder1.appendWord(
            Word(title, Word::Type::Text, MessageColor(MessageColor::Text), QString(), QString()));

        builder1.getMessage()->centered = true;
        emoteChannel->addMessage(builder1.getMessage());

        // EMOTES
        messages::MessageBuilder builder2;
        builder2.getMessage()->centered = true;

        map.each([&](const QString &key, const EmoteData &value) {
            builder2.appendWord(Word(value.image, Word::Type::AlwaysShow, key, emoteDesc,
                                     Link(Link::Type::InsertText, key)));
        });

        emoteChannel->addMessage(builder2.getMessage());
    };

    addEmotes(this->emoteManager.bttvGlobalEmotes, "BetterTTV Global Emotes",
              "BetterTTV Global Emote");
    addEmotes(*channel->bttvChannelEmotes.get(), "BetterTTV Channel Emotes",
              "BetterTTV Channel Emote");
    addEmotes(this->emoteManager.ffzGlobalEmotes, "FrankerFaceZ Global Emotes",
              "FrankerFaceZ Global Emote");
    addEmotes(*channel->ffzChannelEmotes.get(), "FrankerFaceZ Channel Emotes",
              "FrankerFaceZ Channel Emote");

    //    addEmotes(this->emoteManager.getEmojis(), "Emojis", "Emoji");

    this->view->setChannel(emoteChannel);
}
}
}
