// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/notifications/NotificationController.hpp"

#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "Test.hpp"

namespace chatterino {

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    NotificationController *getNotifications() override
    {
        return &this->notifications;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    mock::EmptyLogging logging;
    mock::MockTwitchIrcServer twitch;
    NotificationController notifications;
};

}  // namespace

TEST(NotificationController, DeduplicatesLiveStreams)
{
    MockApplication app;

    const auto notifyLive = [&app](const QString &channelId,
                                   const QString &streamId) {
        app.notifications.notifyTwitchChannelLive({
            .channelId = channelId,
            .streamId = streamId,
            .channelName = channelId,
            .displayName = channelId,
            .title = streamId,
        });
    };
    const auto messages = [&app] {
        return app.twitch.getLiveChannel()->getMessageSnapshot();
    };

    notifyLive("forsen", "stream-1");
    ASSERT_EQ(messages().size(), 1);
    EXPECT_FALSE(messages().at(0)->flags.has(MessageFlag::Disabled));

    notifyLive("forsen", "stream-1");
    EXPECT_EQ(messages().size(), 1);

    notifyLive("forsen", "stream-2");
    ASSERT_EQ(messages().size(), 2);
    EXPECT_TRUE(messages().at(0)->flags.has(MessageFlag::Disabled));
    EXPECT_FALSE(messages().at(1)->flags.has(MessageFlag::Disabled));

    notifyLive("pajlada", "stream-1");
    ASSERT_EQ(messages().size(), 3);
    EXPECT_FALSE(messages().at(2)->flags.has(MessageFlag::Disabled));

    app.notifications.notifyTwitchChannelOffline("forsen");
    EXPECT_TRUE(messages().at(1)->flags.has(MessageFlag::Disabled));
    EXPECT_FALSE(messages().at(2)->flags.has(MessageFlag::Disabled));

    notifyLive("forsen", "stream-2");
    EXPECT_EQ(messages().size(), 3);

    notifyLive("forsen", "stream-3");
    ASSERT_EQ(messages().size(), 4);
    EXPECT_FALSE(messages().at(3)->flags.has(MessageFlag::Disabled));
}

}  // namespace chatterino
