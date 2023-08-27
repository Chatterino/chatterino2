#pragma once

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

/// Adds a timeout or replaces a previous one sent in the last 20 messages and in the last 5s.
/// This function accepts any buffer to store the messsages in.
/// @param replaceMessage A function of type `void (int index, MessagePtr toReplace, MessagePtr replacement)`
///                       - replace `buffer[i]` (=toReplace) with `replacement`
/// @param addMessage A function of type `void (MessagePtr message)`
///                   - adds the `message`.
/// @param disableUserMessages If set, disables all message by the timed out user.
template <typename Buf, typename Replace, typename Add>
void addOrReplaceChannelTimeout(const Buf &buffer, MessagePtr message,
                                QTime now, Replace replaceMessage,
                                Add addMessage, bool disableUserMessages)
{
    // NOTE: This function uses the messages PARSE time to figure out whether they should be replaced
    // This works as expected for incoming messages, but not for historic messages.
    // This has never worked before, but would be nice in the future.
    // For this to work, we need to make sure *all* messages have a "server received time".

    auto snapshotLength = static_cast<qsizetype>(buffer.size());

    auto end = std::max<qsizetype>(0, snapshotLength - 20);

    bool shouldAddMessage = true;

    QTime minimumTime = now.addSecs(-5);

    auto timeoutStackStyle = static_cast<TimeoutStackStyle>(
        getSettings()->timeoutStackStyle.getValue());

    for (auto i = snapshotLength - 1; i >= end; --i)
    {
        const MessagePtr &s = buffer[i];

        if (s->parseTime < minimumTime)
        {
            break;
        }

        if (s->flags.has(MessageFlag::Untimeout) &&
            s->timeoutUser == message->timeoutUser)
        {
            break;
        }

        if (timeoutStackStyle == TimeoutStackStyle::DontStackBeyondUserMessage)
        {
            if (s->loginName == message->timeoutUser &&
                s->flags.hasNone({MessageFlag::Disabled, MessageFlag::Timeout,
                                  MessageFlag::Untimeout}))
            {
                break;
            }
        }

        if (s->flags.has(MessageFlag::Timeout) &&
            s->timeoutUser == message->timeoutUser)
        {
            if (message->flags.has(MessageFlag::PubSub) &&
                !s->flags.has(MessageFlag::PubSub))
            {
                replaceMessage(i, s, message);
                shouldAddMessage = false;
                break;
            }
            if (!message->flags.has(MessageFlag::PubSub) &&
                s->flags.has(MessageFlag::PubSub))
            {
                shouldAddMessage =
                    timeoutStackStyle == TimeoutStackStyle::DontStack;
                break;
            }

            uint32_t count = s->count + 1;

            MessageBuilder replacement(timeoutMessage, message->timeoutUser,
                                       message->loginName, message->searchText,
                                       count);

            replacement->timeoutUser = message->timeoutUser;
            replacement->count = count;
            replacement->flags = message->flags;

            replaceMessage(i, s, replacement.release());

            shouldAddMessage = false;
            break;
        }
    }

    // disable the messages from the user
    if (disableUserMessages)
    {
        for (qsizetype i = 0; i < snapshotLength; i++)
        {
            auto &s = buffer[i];
            if (s->loginName == message->timeoutUser &&
                s->flags.hasNone({MessageFlag::Timeout, MessageFlag::Untimeout,
                                  MessageFlag::Whisper}))
            {
                // FOURTF: disabled for now
                // PAJLADA: Shitty solution described in Message.hpp
                s->flags.set(MessageFlag::Disabled);
            }
        }
    }

    if (shouldAddMessage)
    {
        addMessage(message);
    }
}

}  // namespace chatterino
