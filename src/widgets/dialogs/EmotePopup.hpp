#pragma once

#include "widgets/BaseWindow.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

struct Link;
class ChannelView;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

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
