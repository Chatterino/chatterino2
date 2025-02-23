#include "providers/twitch/eventsub/MessageHandlers.hpp"

#include "Application.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"

namespace chatterino::eventsub {

void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Clear & /*action*/)
{
    runInGuiThread([chan, actor{event.moderatorUserLogin.qt()}, time] {
        chan->addOrReplaceClearChat(
            MessageBuilder::makeClearChatMessage(time, actor), time);
        if (getSettings()->hideModerated)
        {
            // XXX: This is expensive. We could use a layout request if the layout
            //      would store the previous message flags.
            getApp()->getWindows()->forceLayoutChannelViews();
        }
    });
}

}  // namespace chatterino::eventsub
