#include "providers/recentmessages/Api.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/recentmessages/Impl.hpp"
#include "util/PostToThread.hpp"

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoRecentMessages;

}  // namespace

namespace chatterino::recentmessages {

using namespace recentmessages::detail;

void load(
    const QString &channelName, std::weak_ptr<Channel> channelPtr,
    ResultCallback onLoaded, ErrorCallback onError, const int limit,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>
        after,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>
        before,
    const bool jitter)
{
    qCDebug(LOG) << "Loading recent messages for" << channelName;

    const auto url =
        constructRecentMessagesUrl(channelName, limit, after, before);

    const long delayMs = jitter ? std::rand() % 100 : 0;
    QTimer::singleShot(delayMs, [=] {
        NetworkRequest(url)
            .onSuccess([channelPtr, onLoaded](const auto &result) {
                auto shared = channelPtr.lock();
                if (!shared)
                {
                    return;
                }

                qCDebug(LOG) << "Successfully loaded recent messages for"
                             << shared->getName();

                auto root = result.parseJson();
                auto parsedMessages = parseRecentMessages(root);

                // build the Communi messages into chatterino messages
                auto builtMessages =
                    buildRecentMessages(parsedMessages, shared.get());

                postToThread([shared = std::move(shared),
                              root = std::move(root),
                              messages = std::move(builtMessages),
                              onLoaded]() mutable {
                    // Notify user about a possible gap in logs if it returned some messages
                    // but isn't currently joined to a channel
                    const auto errorCode = root.value("error_code").toString();
                    if (!errorCode.isEmpty())
                    {
                        qCDebug(LOG)
                            << QString("Got error from API: error_code=%1, "
                                       "channel=%2")
                                   .arg(errorCode, shared->getName());
                        if (errorCode == "channel_not_joined" &&
                            !messages.empty())
                        {
                            shared->addSystemMessage(
                                "Message history service recovering, there may "
                                "be gaps in the message history.");
                        }
                    }

                    onLoaded(messages);
                });
            })
            .onError([channelPtr, onError](const NetworkResult &result) {
                auto shared = channelPtr.lock();
                if (!shared)
                {
                    return;
                }

                qCDebug(LOG) << "Failed to load recent messages for"
                             << shared->getName();

                shared->addSystemMessage(
                    QStringLiteral(
                        "Message history service unavailable (Error: %1)")
                        .arg(result.formatError()));

                onError();
            })
            .execute();
    });
}

}  // namespace chatterino::recentmessages
