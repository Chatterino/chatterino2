// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/ChannelHelpers.hpp"

#include "mocks/BaseApplication.hpp"
#include "Test.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace chatterino {

TEST(ChannelHelpers, DeduplicateMessages)
{
    auto whisperWithoutID = std::make_shared<Message>();
    whisperWithoutID->flags.set(MessageFlag::Whisper);
    auto systemMessageWithoutID = std::make_shared<Message>();
    systemMessageWithoutID->flags.set(MessageFlag::System);
    systemMessageWithoutID->messageText = "connected";
    auto clonedSystemMessageWithoutID = systemMessageWithoutID->clone();
    auto messageWithID = std::make_shared<Message>();
    messageWithID->id = "message-id";
    auto clonedMessageWithID = messageWithID->clone();

    std::vector<MessagePtr> messages{
        whisperWithoutID,
        messageWithID,
        systemMessageWithoutID,
        whisperWithoutID,
        clonedSystemMessageWithoutID,
        clonedMessageWithID,
        systemMessageWithoutID,
        whisperWithoutID,
    };
    deduplicateMessages(messages);

    ASSERT_EQ(messages.size(), 4);

    // The same Message object should only be kept once
    EXPECT_EQ(std::ranges::count(messages, whisperWithoutID), 1);
    EXPECT_EQ(std::ranges::count(messages, systemMessageWithoutID), 1);

    // Different Message objects without IDs should be kept even if they
    // have the same content
    EXPECT_EQ(std::ranges::count(messages, clonedSystemMessageWithoutID), 1);

    // Different Message objects with the same ID should be deduplicated
    EXPECT_EQ(std::ranges::count(messages, messageWithID) +
                  std::ranges::count(messages, clonedMessageWithID),
              1);
}

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
