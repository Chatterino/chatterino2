#pragma once

#include "channel.hpp"
#include "widgets/basewindow.hpp"
#include "widgets/helper/channelview.hpp"

namespace chatterino {
namespace widgets {

class EmotePopup : public BaseWindow
{
public:
    explicit EmotePopup(singletons::ThemeManager &);

    void loadChannel(SharedChannel channel);
    void loadEmojis();

private:
    ChannelView *viewEmotes;
    ChannelView *viewEmojis;
};

}  // namespace widgets
}  // namespace chatterino
