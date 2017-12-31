#pragma once

#include "channel.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/channelview.hpp"

namespace chatterino {
namespace widgets {

class EmotePopup : public BaseWidget
{
public:
    explicit EmotePopup(singletons::ThemeManager &);

    void loadChannel(std::shared_ptr<Channel> channel);
    void loadEmojis();

private:
    ChannelView *viewEmotes;
    ChannelView *viewEmojis;
};

}  // namespace widgets
}  // namespace chatterino
