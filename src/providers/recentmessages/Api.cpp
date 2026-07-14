// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/recentmessages/Api.hpp"

#include "Application.hpp"
#include "common/Env.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/recentmessages/Impl.hpp"
#include "providers/recentmessages/Provider.hpp"
#include "singletons/Settings.hpp"
#include "util/PostToThread.hpp"

#include <algorithm>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoRecentMessages;
constexpr int REQUEST_TIMEOUT_MS = 10 * 1000;

}  // namespace

namespace chatterino::recentmessages {

using namespace recentmessages::detail;

namespace {

struct PartialResponse {
    QJsonObject root;
    QString providerUrl;
};

struct LoadContext {
    QString channelName;
    std::weak_ptr<Channel> channelPtr;
    ResultCallback onLoaded;
    ErrorCallback onError;
    int limit;
    std::optional<std::chrono::time_point<std::chrono::system_clock>> after;
    std::optional<std::chrono::time_point<std::chrono::system_clock>> before;
    std::vector<Provider> providers;
    size_t nextProvider{};
    size_t failedProviders{};
    QString lastError = "No valid providers enabled";
    std::optional<PartialResponse> partialResponse;
};

bool hasRemainingEnabledProvider(const std::shared_ptr<LoadContext> &context)
{
    return std::ranges::any_of(
        context->providers.begin() +
            static_cast<std::ptrdiff_t>(context->nextProvider),
        context->providers.end(), &Provider::enabled);
}

void reportProviderIssue(const std::shared_ptr<LoadContext> &context,
                         QString message)
{
    auto shared = context->channelPtr.lock();
    if (!shared)
    {
        return;
    }

    if (hasRemainingEnabledProvider(context))
    {
        message += QStringLiteral(" Trying the next provider.");
    }
    shared->addSystemMessage(message);
}

void reportProviderFailure(const std::shared_ptr<LoadContext> &context,
                           const QString &providerUrl, const QString &error)
{
    reportProviderIssue(
        context,
        QStringLiteral("Message history request to %1 failed (Error: %2).")
            .arg(providerUrl, error));
}

void finishLoad(const std::shared_ptr<LoadContext> &context,
                const QJsonObject &root, const QString &providerUrl,
                bool showGapWarning)
{
    auto shared = context->channelPtr.lock();
    if (!shared)
    {
        return;
    }

    qCDebug(LOG) << "Successfully loaded recent messages for"
                 << shared->getName() << "from" << providerUrl;

    auto parsedMessages = parseRecentMessages(root);
    auto builtMessages = buildRecentMessages(parsedMessages, shared.get());
    const bool usedFallback = context->failedProviders > 0;

    postToThread([shared = std::move(shared),
                  messages = std::move(builtMessages),
                  onLoaded = context->onLoaded, providerUrl, showGapWarning,
                  usedFallback]() mutable {
        assert(!isAppAboutToQuit());
        if (usedFallback && !showGapWarning)
        {
            shared->addSystemMessage(
                QStringLiteral("Message history was loaded from %1")
                    .arg(providerUrl));
        }
        if (showGapWarning && !messages.empty())
        {
            shared->addSystemMessage(
                "Message history service recovering, there may be gaps in "
                "the message history.");
        }
        onLoaded(messages);
    });
}

void tryLoad(const std::shared_ptr<LoadContext> &context)
{
    if (isAppAboutToQuit() || context->channelPtr.expired())
    {
        return;
    }

    const auto nextProvider = recentmessages::nextEnabledProvider(
        context->providers, context->nextProvider);
    if (!nextProvider)
    {
        if (context->partialResponse)
        {
            finishLoad(context, context->partialResponse->root,
                       context->partialResponse->providerUrl, true);
            return;
        }

        auto shared = context->channelPtr.lock();
        if (!shared)
        {
            return;
        }

        shared->addSystemMessage(
            QStringLiteral("Message history service unavailable (Error: %1)")
                .arg(context->lastError));
        context->onError();
        return;
    }

    const auto &provider = *nextProvider;
    if (!provider.isValid())
    {
        context->lastError = QStringLiteral("Invalid provider URL");
        context->failedProviders++;
        reportProviderIssue(
            context,
            QStringLiteral("Message history provider URL is invalid: %1.")
                .arg(provider.url()));
        tryLoad(context);
        return;
    }

    const auto url = constructRecentMessagesUrl(
        provider.url(), context->channelName, context->limit, context->after,
        context->before);

    qCDebug(LOG) << "Loading recent messages for" << context->channelName
                 << "from" << provider.url();

    NetworkRequest(url)
        .timeout(REQUEST_TIMEOUT_MS)
        .onSuccess([context, provider](const auto &result) {
            assert(!isAppAboutToQuit());

            auto root = result.parseJson();
            const auto responseType = classifyRecentMessagesResponse(root);
            if (responseType != ResponseType::Complete)
            {
                const auto errorCode = root.value("error_code").toString();
                const auto error = root.value("error").toString();
                if (!errorCode.isEmpty())
                {
                    context->lastError = errorCode;
                }
                else if (!error.isEmpty())
                {
                    context->lastError = error;
                }
                else
                {
                    context->lastError = QStringLiteral("Invalid response");
                }
                if (responseType == ResponseType::Partial &&
                    !context->partialResponse)
                {
                    context->partialResponse = PartialResponse{
                        .root = root,
                        .providerUrl = provider.url(),
                    };
                }
                qCDebug(LOG) << "Failed to load recent messages for"
                             << context->channelName << "from" << provider.url()
                             << context->lastError;
                context->failedProviders++;
                reportProviderFailure(context, provider.url(),
                                      context->lastError);
                tryLoad(context);
                return;
            }

            finishLoad(context, root, provider.url(), false);
        })
        .onError([context, provider](const NetworkResult &result) {
            assert(!isAppAboutToQuit());

            context->lastError = result.formatError();
            qCDebug(LOG) << "Failed to load recent messages for"
                         << context->channelName << "from" << provider.url()
                         << context->lastError;
            context->failedProviders++;
            reportProviderFailure(context, provider.url(), context->lastError);
            tryLoad(context);
        })
        .execute();
}

std::vector<Provider> configuredProviders()
{
    const auto &env = Env::get();
    if (!env.recentMessagesApiUrl.isEmpty())
    {
        return {{env.recentMessagesApiUrl, true}};
    }

    const auto configured = getSettings()->recentMessagesProviders.readOnly();
    return {configured->begin(), configured->end()};
}

}  // namespace

void load(
    const QString &channelName, std::weak_ptr<Channel> channelPtr,
    ResultCallback onLoaded, ErrorCallback onError, const int limit,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>
        after,
    const std::optional<std::chrono::time_point<std::chrono::system_clock>>
        before,
    const bool jitter)
{
    auto context = std::make_shared<LoadContext>(LoadContext{
        .channelName = channelName,
        .channelPtr = std::move(channelPtr),
        .onLoaded = std::move(onLoaded),
        .onError = std::move(onError),
        .limit = limit,
        .after = after,
        .before = before,
        .providers = configuredProviders(),
        .partialResponse = std::nullopt,
    });

    // we don't need a strong RNG here
    // NOLINTNEXTLINE(cert-*)
    const long delayMs = jitter ? std::rand() % 100 : 0;
    QTimer::singleShot(delayMs, [context = std::move(context)] {
        tryLoad(context);
    });
}

}  // namespace chatterino::recentmessages
