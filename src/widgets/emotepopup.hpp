#pragma once

#include "channel.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/channelview.hpp"

namespace chatterino {
namespace widgets {

class EmotePopup : public BaseWidget
{
public:
    explicit EmotePopup(ColorScheme &);

    void loadChannel(std::shared_ptr<Channel> channel);

private:
    ChannelView *view;
};

}  // namespace widgets
}  // namespace chatterino
