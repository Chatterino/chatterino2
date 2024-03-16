#include "common/network/NetworkRequest.hpp"

#include "common/network/NetworkManager.hpp"
#include "common/network/NetworkResult.hpp"

#include <gtest/gtest.h>
#include <QCoreApplication>

using namespace chatterino;

namespace {

#ifdef CHATTERINO_TEST_USE_PUBLIC_HTTPBIN
// Not using httpbin.org, since it can be really slow and cause timeouts.
// postman-echo has the same API.
const char *const HTTPBIN_BASE_URL = "https://postman-echo.com";
#else
const char *const HTTPBIN_BASE_URL = "http://127.0.0.1:9051";
#endif

QString getStatusURL(int code)
{
    return QString("%1/status/%2").arg(HTTPBIN_BASE_URL).arg(code);
}

QString getDelayURL(int delay)
{
    return QString("%1/delay/%2").arg(HTTPBIN_BASE_URL).arg(delay);
}

class RequestWaiter
{
public:
    void requestDone()
    {
        {
            std::unique_lock lck(this->mutex_);
            ASSERT_FALSE(this->requestDone_);
            this->requestDone_ = true;
        }
        this->condition_.notify_one();
    }

    void waitForRequest()
    {
        using namespace std::chrono_literals;

        while (true)
        {
            {
                std::unique_lock lck(this->mutex_);
                bool done = this->condition_.wait_for(lck, 10ms, [this] {
                    return this->requestDone_;
                });
                if (done)
                {
                    break;
                }
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool requestDone_ = false;
};

}  // namespace

TEST(NetworkRequest, Success)
{
    const std::vector<int> codes{200, 201, 202, 203, 204, 205, 206};

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnSuccess)
{
    const std::vector<int> codes{200, 201, 202, 203, 204, 205, 206};

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnError)
{
    const std::vector<int> codes{
        400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410,
        411, 412, 413, 414, 418, 500, 501, 502, 503, 504,
    };

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, TimeoutNotTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnTimeout)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

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
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}
