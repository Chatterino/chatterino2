#include "messages/MessageSimilarity.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/LimitedQueueSnapshot.hpp"  // IWYU pragma: keep
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Settings.hpp"

#include <algorithm>
#include <vector>

namespace {

using namespace chatterino;

float relativeSimilarity(QStringView str1, QStringView str2)
{
    using SizeType = QStringView::size_type;

    // Longest Common Substring Problem
    std::vector<std::vector<int>> tree(str1.size(),
                                       std::vector<int>(str2.size(), 0));
    int z = 0;

    for (SizeType i = 0; i < str1.size(); ++i)
    {
        for (SizeType j = 0; j < str2.size(); ++j)
        {
            if (str1[i] == str2[j])
            {
                if (i == 0 || j == 0)
                {
                    tree[i][j] = 1;
                }
                else
                {
                    tree[i][j] = tree[i - 1][j - 1] + 1;
                }
                z = std::max(tree[i][j], z);
            }
            else
            {
                tree[i][j] = 0;
            }
        }
    }

    // ensure that no div by 0
    if (z == 0)
    {
        return 0.F;
    }

    auto div = std::max<>({static_cast<SizeType>(1), str1.size(), str2.size()});

    return float(z) / float(div);
}

template <std::ranges::bidirectional_range T>
float inMessages(const MessagePtr &msg, const T &messages)
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

}  // namespace

namespace chatterino {

template <std::ranges::bidirectional_range T>
void setSimilarityFlags(const MessagePtr &message, const T &messages)
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

        if (inMessages(message, messages) > getSettings()->similarityPercentage)
        {
            message->flags.set(MessageFlag::Similar);
            if (getSettings()->colorSimilarDisabled)
            {
                message->flags.set(MessageFlag::Disabled);
            }
        }
    }
}

template void setSimilarityFlags<std::vector<MessagePtr>>(
    const MessagePtr &msg, const std::vector<MessagePtr> &messages);
template void setSimilarityFlags<LimitedQueueSnapshot<MessagePtr>>(
    const MessagePtr &msg, const LimitedQueueSnapshot<MessagePtr> &messages);

}  // namespace chatterino
