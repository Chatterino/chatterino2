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
        bool firstPubSub;
        bool secondPubSub;
    };
    const std::array cases = {
        Case{.name = "IRC then IRC",
             .firstPubSub = false,
             .secondPubSub = false},
        Case{.name = "EventSub then EventSub",
             .firstPubSub = true,
             .secondPubSub = true},
        Case{.name = "IRC then EventSub",
             .firstPubSub = false,
             .secondPubSub = true},
        Case{.name = "EventSub then IRC",
             .firstPubSub = true,
             .secondPubSub = false},
    };
    for (const auto &test : cases)
    {
        SCOPED_TRACE(test.name);
        auto userMessage = std::make_shared<Message>();
        userMessage->loginName = "user";
        userMessage->serverReceivedTime = time;
        std::vector<MessagePtr> messages{userMessage};

        const auto addTimeout = [&](bool pubSub) {
            auto message = std::make_shared<Message>();
            message->timeoutUser = "user";
            message->serverReceivedTime = time;
            message->flags.set(MessageFlag::Timeout,
                               MessageFlag::ModerationAction);
            if (pubSub)
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

        auto first = addTimeout(test.firstPubSub);
        auto second = addTimeout(test.secondPubSub);
        EXPECT_TRUE(userMessage->flags.has(MessageFlag::Disabled));
        EXPECT_TRUE(userMessage->flags.has(MessageFlag::InvalidReplyTarget));

        if (test.firstPubSub == test.secondPubSub)
        {
            ASSERT_EQ(messages.size(), 3);
            EXPECT_EQ(messages.at(1), first);
            EXPECT_EQ(messages.at(2), second);
        }
        else
        {
            ASSERT_EQ(messages.size(), 2);
            EXPECT_EQ(messages.at(1), test.firstPubSub ? first : second);
        }
    }
}

}  // namespace chatterino
