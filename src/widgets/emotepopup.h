#pragma once

#include "channel.hpp"
#include "emotemanager.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/channelview.hpp"

namespace chatterino {
namespace widgets {

class EmotePopup : public BaseWidget
{
public:
    explicit EmotePopup(ColorScheme &, EmoteManager &);

    void loadChannel(std::shared_ptr<Channel> channel);

private:
    ChannelView *view;
    EmoteManager &emoteManager;
};
}
}
