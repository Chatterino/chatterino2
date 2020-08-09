#pragma once

#include "widgets/BasePopup.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

struct Link;
class ChannelView;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class EmotePopup : public BasePopup
{
public:
    EmotePopup(QWidget *parent = nullptr);

    void loadChannel(ChannelPtr channel);
    void loadEmojis();

    virtual void closeEvent(QCloseEvent *event) override;

    pajlada::Signals::Signal<Link> linkClicked;

private:
    ChannelView *globalEmotesView_{};
    ChannelView *channelEmotesView_{};
    ChannelView *subEmotesView_{};
    ChannelView *viewEmojis_{};
};

}  // namespace chatterino
