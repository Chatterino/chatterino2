// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/notifications/NotificationController.hpp"

#include "common/enums/UsernameDisplayMode.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Link.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "Test.hpp"

#include <array>
#include <memory>
#include <optional>

using namespace Qt::Literals;

namespace chatterino {

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

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

    AccountController accounts;
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

TEST(NotificationController, StatusMessagesRespectUsernameStyle)
{
    struct TestCase {
        UsernameDisplayMode mode;
        QString displayName;
        QString expected;
    };

    const std::array<TestCase, 6> cases{{
        // Localized usernames respect the username display mode setting
        {
            .mode = UsernameDisplayMode::Username,
            .displayName = u"福森"_s,
            .expected = u"chinese_forsen"_s,
        },
        {
            .mode = UsernameDisplayMode::LocalizedName,
            .displayName = u"福森"_s,
            .expected = u"福森"_s,
        },
        {
            .mode = UsernameDisplayMode::UsernameAndLocalizedName,
            .displayName = u"福森"_s,
            .expected = u"chinese_forsen (福森)"_s,
        },

        // Case-only display names use the login
        {
            .mode = UsernameDisplayMode::Username,
            .displayName = u"cHiNeSe_FoRsEn"_s,
            .expected = u"chinese_forsen"_s,
        },
        {
            .mode = UsernameDisplayMode::LocalizedName,
            .displayName = u"cHiNeSe_FoRsEn"_s,
            .expected = u"chinese_forsen"_s,
        },
        {
            .mode = UsernameDisplayMode::UsernameAndLocalizedName,
            .displayName = u"cHiNeSe_FoRsEn"_s,
            .expected = u"chinese_forsen"_s,
        },
    }};

    for (const auto &test : cases)
    {
        // Usernames are formatted the same with and without the stream title
        for (bool showTitle : {false, true})
        {
            SCOPED_TRACE(::testing::Message()
                         << test.mode << ", " << test.displayName << ", "
                         << showTitle);
            MockApplication app;
            app.settings.usernameDisplayMode.setValue(test.mode);
            app.settings.showTitleInLiveMessage.setValue(showTitle);

            // Live notifications for fake channels use the selected display mode
            app.notifications.notifyTwitchChannelLive({
                .channelId = u"22484632"_s,
                .streamId = u"stream-1"_s,
                .channelName = u"chinese_forsen"_s,
                .displayName = test.displayName,
                .title = u"游戏与便便！"_s,
            });

            const auto checkMessage = [&](const MessagePtr &message,
                                          const QString &text) {
                ASSERT_NE(message, nullptr);
                ASSERT_GE(message->elements.size(), 2);

                // The displayed name matches the selected display mode
                const auto *name = dynamic_cast<const TextElement *>(
                    message->elements.at(1).get());
                ASSERT_NE(name, nullptr);
                EXPECT_EQ(name->words().join(' '), test.expected);

                // The link always points at channel login
                EXPECT_EQ(name->getLink().type, Link::UserInfo);
                EXPECT_EQ(name->getLink().value, u"chinese_forsen"_s);

                // The name is a username element
                EXPECT_EQ(name->getFlags(),
                          MessageElementFlags{MessageElementFlag::Username});

                // The displayed name is used in the plain and searchable message text
                EXPECT_EQ(message->messageText, text);
                EXPECT_EQ(message->searchText, text);

                // Channel ID survives
                EXPECT_EQ(message->id, u"22484632"_s);
            };

            const auto liveText =
                test.expected +
                (showTitle ? u" is live: 游戏与便便！"_s : u" is live!"_s);
            const auto messages =
                app.twitch.getLiveChannel()->getMessageSnapshot();
            ASSERT_EQ(messages.size(), 1);
            checkMessage(messages.at(0), liveText);

            auto channel = std::make_shared<TwitchChannel>(u"chinese_forsen"_s);
            channel->setRoomId(u"22484632"_s);
            HelixStream stream;
            stream.id = u"stream-2"_s;
            stream.userId = u"22484632"_s;
            stream.userLogin = u"chinese_forsen"_s;
            stream.userName = test.displayName;
            stream.title = u"游戏与便便！"_s;
            stream.startedAt = u"1984-04-20T21:37:00Z"_s;

            // Live status messages in real channels use the selected display mode
            channel->updateStreamStatus(stream, true);
            checkMessage(channel->getLastMessage(), liveText);

            // Events mirrored to the /live tab use the selected display mode
            const auto joinedMessages =
                app.twitch.getLiveChannel()->getMessageSnapshot();
            ASSERT_EQ(joinedMessages.size(), 2);
            checkMessage(joinedMessages.at(1), liveText);

            // Offline status messages use the selected display mode
            channel->updateStreamStatus(std::nullopt, false);
            const auto offline = channel->getLastMessage();
            checkMessage(offline, test.expected + u" is now offline."_s);
        }
    }
}

}  // namespace chatterino
