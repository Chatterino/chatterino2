#pragma once

#include "common/Channel.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

class EmotePopup : public BaseWindow
{
public:
    EmotePopup();

    void loadChannel(ChannelPtr channel);
    void loadEmojis();

    pajlada::Signals::Signal<Link> linkClicked;

private:
    ChannelView *globalEmotesView_{};
    ChannelView *channelEmotesView_{};
    ChannelView *subEmotesView_{};
    ChannelView *viewEmojis_{};
};

}  // namespace chatterino
