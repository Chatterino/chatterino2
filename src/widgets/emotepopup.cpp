#include "emotepopup.h"
namespace chatterino {
namespace widgets {

EmotePopup::EmotePopup(QWidget *parent)
    : QWidget(parent)
{
}

void EmotePopup::loadChannel(std::shared_ptr<Channel> channel)
{
    //    channel->bttvChannelEmotes.each([](const QString &key, const EmoteData &value) {

    //        //
    //    });
}
}
}
