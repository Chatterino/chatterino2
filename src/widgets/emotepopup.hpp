#pragma once

#include "channel.hpp"
#include "widgets/basewindow.hpp"
#include "widgets/helper/channelview.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace widgets {

class EmotePopup : public BaseWindow
{
public:
    EmotePopup();

    void loadChannel(ChannelPtr channel);
    void loadEmojis();

    pajlada::Signals::Signal<messages::Link> linkClicked;

private:
    ChannelView *viewEmotes;
    ChannelView *viewEmojis;
};

}  // namespace widgets
}  // namespace chatterino
