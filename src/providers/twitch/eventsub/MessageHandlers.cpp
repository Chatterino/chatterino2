#include "providers/twitch/eventsub/MessageHandlers.hpp"

#include "Application.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/eventsub/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/FormatTime.hpp"
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

void handleModerateMessage(
    TwitchChannel *chan, const QDateTime &time,
    const lib::payload::channel_moderate::v2::Event &event,
    const lib::payload::channel_moderate::v2::Timeout &action)
{
    // Not all compilers support QDateTime::toStdSysMilliseconds
    std::chrono::system_clock::time_point chronoTime{
        std::chrono::milliseconds{time.toMSecsSinceEpoch()}};

    auto duration =
        std::chrono::round<std::chrono::seconds>(action.expiresAt - chronoTime);

    EventSubMessageBuilder builder(chan, time);
    builder->loginName = event.moderatorUserLogin.qt();

    QString text;
    bool isShared = event.isFromSharedChat();

    builder.appendUser(event.moderatorUserName, event.moderatorUserLogin, text);
    builder.emplaceSystemTextAndUpdate("timed out", text);
    builder.appendUser(action.userName, action.userLogin, text);

    builder.emplaceSystemTextAndUpdate("for", text);
    builder
        .emplaceSystemTextAndUpdate(
            formatTime(static_cast<int>(duration.count())), text)
        ->setTrailingSpace(isShared);

    if (isShared)
    {
        builder.emplaceSystemTextAndUpdate("in", text);
        builder.appendUser(*event.sourceBroadcasterUserName,
                           *event.sourceBroadcasterUserLogin, text, false);
    }

    if (action.reason.view().empty())
    {
        builder.emplaceSystemTextAndUpdate(".", text);
    }
    else
    {
        builder.emplaceSystemTextAndUpdate(":", text);
        builder.emplaceSystemTextAndUpdate(action.reason.qt(), text);
    }

    builder->messageText = text;
    builder->searchText = text;
    builder->timeoutUser = action.userLogin.qt();

    auto msg = builder.release();
    runInGuiThread([chan, msg] {
        // TODO: addOrReplaceTimeout (doesn't work with shared chat yet)
        chan->addMessage(msg, MessageContext::Original);
    });
}

}  // namespace chatterino::eventsub
