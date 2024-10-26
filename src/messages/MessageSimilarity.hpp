#pragma once

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Settings.hpp"

#include <ranges>

namespace chatterino::similarity::detail {

float relativeSimilarity(QStringView str1, QStringView str2);

float inMessages(const MessagePtr &msg,
                 const std::ranges::bidirectional_range auto &messages)
{
    float similarityPercent = 0.0F;

    for (const auto &prevMsg :
         messages | std::views::reverse |
             std::views::take(getSettings()->hideSimilarMaxMessagesToCheck))
    {
        if (prevMsg->parseTime.secsTo(QTime::currentTime()) >=
            getSettings()->hideSimilarMaxDelay)
        {
            break;
        }
        if (getSettings()->hideSimilarBySameUser &&
            msg->loginName != prevMsg->loginName)
        {
            continue;
        }
        similarityPercent = std::max(
            similarityPercent,
            relativeSimilarity(msg->messageText, prevMsg->messageText));
    }

    return similarityPercent;
}

}  // namespace chatterino::similarity::detail

namespace chatterino {

void setSimilarityFlags(const MessagePtr &message,
                        const std::ranges::bidirectional_range auto &messages)
{
    if (getSettings()->similarityEnabled)
    {
        bool isMyself =
            message->loginName ==
            getApp()->getAccounts()->twitch.getCurrent()->getUserName();
        bool hideMyself = getSettings()->hideSimilarMyself;

        if (isMyself && !hideMyself)
        {
            return;
        }

        if (similarity::detail::inMessages(message, messages) >
            getSettings()->similarityPercentage)
        {
            message->flags.set(MessageFlag::Similar);
            if (getSettings()->colorSimilarDisabled)
            {
                message->flags.set(MessageFlag::Disabled);
            }
        }
    }
}

}  // namespace chatterino
