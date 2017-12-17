#pragma once

#include "channel.hpp"
#include "emotemanager.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/channelview.hpp"

namespace chatterino {
namespace widgets {

class EmotePopup : public BaseWidget
{
public:
    explicit EmotePopup(ColorScheme &, EmoteManager &, WindowManager &);

    void loadChannel(std::shared_ptr<Channel> channel);

private:
    ChannelView *view;
    EmoteManager &emoteManager;
};

}  // namespace widgets
}  // namespace chatterino
