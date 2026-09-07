// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/ChannelHelpers.hpp"

#include "mocks/BaseApplication.hpp"
#include "Test.hpp"

#include <array>
#include <memory>
#include <vector>

namespace chatterino {

TEST(ChannelHelpers, DontStackTimeouts)
{
    mock::BaseApplication app;
    app.settings.timeoutStackStyle =
        static_cast<int>(TimeoutStackStyle::DontStack);
    const auto time =
        QDateTime::fromString("1984-04-20T21:37:00Z", Qt::ISODate);

    struct Case {
        const char *name;
        bool firstEventSub;
        bool secondEventSub;
    };
    const std::array cases = {
        Case{
            .name = "IRC then IRC",
            .firstEventSub = false,
            .secondEventSub = false,
        },
        Case{
            .name = "EventSub then EventSub",
            .firstEventSub = true,
            .secondEventSub = true,
        },
        Case{
            .name = "IRC then EventSub",
            .firstEventSub = false,
            .secondEventSub = true,
        },
        Case{
            .name = "EventSub then IRC",
            .firstEventSub = true,
            .secondEventSub = false,
        },
    };
    for (const auto &test : cases)
    {
        SCOPED_TRACE(test.name);
        auto userMessage = std::make_shared<Message>();
        userMessage->loginName = "user";
        userMessage->serverReceivedTime = time;
        std::vector<MessagePtr> messages{userMessage};

        const auto addTimeout = [&](bool eventSub) {
            auto message = std::make_shared<Message>();
            message->timeoutUser = "user";
            message->serverReceivedTime = time;
            message->flags.set(MessageFlag::Timeout,
                               MessageFlag::ModerationAction);
            if (eventSub)
            {
                message->flags.set(MessageFlag::PubSub);
            }
            addOrReplaceChannelTimeout(
                messages, message, time,
                [&](auto index, const auto & /*oldMessage*/,
                    const auto &replacement) {
                    messages.at(index) = replacement;
                },
                [&](const auto &added) {
                    messages.push_back(added);
                },
                true);
            return message;
        };

        auto first = addTimeout(test.firstEventSub);
        auto second = addTimeout(test.secondEventSub);
        // The user's messages are still disabled when stacking is disabled.
        EXPECT_TRUE(userMessage->flags.hasAll(MessageFlag::Disabled,
                                              MessageFlag::InvalidReplyTarget));

        if (test.firstEventSub == test.secondEventSub)
        {
            // IRC then IRC and EventSub then EventSub don't stack when stacking is disabled.
            ASSERT_EQ(messages.size(), 3);
            EXPECT_EQ(messages.at(1), first);
            EXPECT_EQ(messages.at(2), second);
        }
        else
        {
            // The same timeout received through IRC and EventSub is deduplicated.
            // Both arrival orders keep the EventSub message.
            ASSERT_EQ(messages.size(), 2);
            EXPECT_EQ(messages.at(1), test.firstEventSub ? first : second);
        }
    }
}

}  // namespace chatterino
