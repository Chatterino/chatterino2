#include "common/network/NetworkRequest.hpp"

#include "common/network/NetworkManager.hpp"
#include "common/network/NetworkResult.hpp"
#include "NetworkHelpers.hpp"
#include "Test.hpp"

#include <QCoreApplication>

using namespace chatterino;

namespace {

QString getStatusURL(int code)
{
    return QString("%1/status/%2").arg(HTTPBIN_BASE_URL).arg(code);
}

QString getDelayURL(int delay)
{
    return QString("%1/delay/%2").arg(HTTPBIN_BASE_URL).arg(delay);
}

}  // namespace

TEST(NetworkRequest, Success)
{
    const std::vector<int> codes{200, 201, 202, 203, 204, 205, 206};

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    for (const auto code : codes)
    {
        auto url = getStatusURL(code);
        RequestWaiter waiter;

        NetworkRequest(url)
            .onSuccess([code, &waiter, url](const NetworkResult &result) {
                EXPECT_EQ(result.status(), code);
                waiter.requestDone();
            })
            .onError([&](const NetworkResult & /*result*/) {
                // The codes should *not* throw an error
                EXPECT_TRUE(false);
                waiter.requestDone();
            })
            .execute();

        waiter.waitForRequest();
    }

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnSuccess)
{
    const std::vector<int> codes{200, 201, 202, 203, 204, 205, 206};

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    for (const auto code : codes)
    {
        auto url = getStatusURL(code);
        RequestWaiter waiter;

        bool finallyCalled = false;

        NetworkRequest(url)
            .finally([&waiter, &finallyCalled] {
                finallyCalled = true;
                waiter.requestDone();
            })
            .execute();

        waiter.waitForRequest();

        EXPECT_TRUE(finallyCalled);
    }
}

TEST(NetworkRequest, Error)
{
    const std::vector<int> codes{
        400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
        411, 412, 413, 414, 418, 500, 501, 502, 503, 504,
    };

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    for (const auto code : codes)
    {
        auto url = getStatusURL(code);
        RequestWaiter waiter;

        NetworkRequest(url)
            .onSuccess([&waiter, url](const NetworkResult & /*result*/) {
                // The codes should throw an error
                EXPECT_TRUE(false);
                waiter.requestDone();
            })
            .onError([code, &waiter, url](const NetworkResult &result) {
                EXPECT_EQ(result.status(), code);

                waiter.requestDone();
            })
            .execute();

        waiter.waitForRequest();
    }

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnError)
{
    const std::vector<int> codes{
        400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
        411, 412, 413, 414, 418, 500, 501, 502, 503, 504,
    };

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    for (const auto code : codes)
    {
        auto url = getStatusURL(code);
        RequestWaiter waiter;

        bool finallyCalled = false;

        NetworkRequest(url)
            .finally([&waiter, &finallyCalled] {
                finallyCalled = true;
                waiter.requestDone();
            })
            .execute();

        waiter.waitForRequest();

        EXPECT_TRUE(finallyCalled);
    }
}

TEST(NetworkRequest, TimeoutTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    auto url = getDelayURL(5);
    RequestWaiter waiter;

    NetworkRequest(url)
        .timeout(1000)
        .onSuccess([&waiter](const NetworkResult & /*result*/) {
            // The timeout should throw an error
            EXPECT_TRUE(false);
            waiter.requestDone();
        })
        .onError([&waiter, url](const NetworkResult &result) {
            qDebug() << QTime::currentTime().toString()
                     << "timeout request finish error";
            EXPECT_EQ(result.error(),
                      NetworkResult::NetworkError::TimeoutError);
            EXPECT_EQ(result.status(), std::nullopt);

            waiter.requestDone();
        })
        .execute();

    waiter.waitForRequest();

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());
}

TEST(NetworkRequest, TimeoutNotTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    auto url = getDelayURL(1);
    RequestWaiter waiter;

    NetworkRequest(url)
        .timeout(3000)
        .onSuccess([&waiter, url](const NetworkResult &result) {
            EXPECT_EQ(result.status(), 200);

            waiter.requestDone();
        })
        .onError([&waiter, url](const NetworkResult & /*result*/) {
            // The timeout should *not* throw an error
            EXPECT_TRUE(false);
            waiter.requestDone();
        })
        .execute();

    waiter.waitForRequest();

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnTimeout)
{
    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    auto url = getDelayURL(5);

    RequestWaiter waiter;
    bool finallyCalled = false;
    bool onSuccessCalled = false;
    bool onErrorCalled = false;

    NetworkRequest(url)
        .timeout(1000)
        .onSuccess([&](const NetworkResult & /*result*/) {
            onSuccessCalled = true;
        })
        .onError([&](const NetworkResult &result) {
            onErrorCalled = true;
            EXPECT_EQ(result.error(),
                      NetworkResult::NetworkError::TimeoutError);
            EXPECT_EQ(result.status(), std::nullopt);
        })
        .finally([&] {
            finallyCalled = true;
            waiter.requestDone();
        })
        .execute();

    waiter.waitForRequest();

    EXPECT_TRUE(finallyCalled);
    EXPECT_TRUE(onErrorCalled);
    EXPECT_FALSE(onSuccessCalled);
    EXPECT_TRUE(NetworkManager::workerThread->isRunning());
}

/// Ensure timeouts don't expire early just because their request took a bit longer to actually fire
///
/// We need to ensure all requests are "executed" before we start waiting for them
TEST(NetworkRequest, BatchedTimeouts)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    // roughly num network manager worker threads * 2
    static const auto numRequests = 10;

    struct RequestState {
        RequestWaiter waiter;
        bool errored = false;
    };

    EXPECT_TRUE(NetworkManager::workerThread->isRunning());

    std::vector<std::shared_ptr<RequestState>> states;

    for (auto i = 1; i <= numRequests; ++i)
    {
        auto state = std::make_shared<RequestState>();

        auto url = getDelayURL(1);

        NetworkRequest(url)
            .timeout(1500)
            .onError([=](const NetworkResult &result) {
                (void)result;
                state->errored = true;
            })
            .finally([=] {
                state->waiter.requestDone();
            })
            .execute();

        states.emplace_back(state);
    }

    for (const auto &state : states)
    {
        state->waiter.waitForRequest();
        EXPECT_FALSE(state->errored);
    }
#endif
}
