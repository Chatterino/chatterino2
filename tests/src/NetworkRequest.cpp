#include "common/NetworkRequest.hpp"
#include "common/NetworkManager.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "providers/twitch/api/Helix.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

#include <gtest/gtest.h>

using namespace chatterino;

namespace {

// Change to http://httpbin.org if you don't want to run the docker image yourself to test this
const char *const HTTPBIN_BASE_URL = "http://127.0.0.1:9051";

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

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    for (const auto code : codes)
    {
        auto url = getStatusURL(code);
        std::mutex mut;
        bool requestDone = false;
        std::condition_variable requestDoneCondition;

        NetworkRequest(url)
            .onSuccess([code, &mut, &requestDone, &requestDoneCondition,
                        url](NetworkResult result) -> Outcome {
                EXPECT_EQ(result.status(), code);

                {
                    std::unique_lock lck(mut);
                    requestDone = true;
                }
                requestDoneCondition.notify_one();
                return Success;
            })
            .onError([&](NetworkResult result) {
                // The codes should *not* throw an error
                EXPECT_TRUE(false);

                {
                    std::unique_lock lck(mut);
                    requestDone = true;
                }
                requestDoneCondition.notify_one();
            })
            .execute();

        // Wait for the request to finish
        std::unique_lock lck(mut);
        requestDoneCondition.wait(lck, [&requestDone] {
            return requestDone;
        });
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
        std::mutex mut;
        bool requestDone = false;
        std::condition_variable requestDoneCondition;

        bool finallyCalled = false;

        NetworkRequest(url)
            .finally(
                [&mut, &requestDone, &requestDoneCondition, &finallyCalled] {
                    finallyCalled = true;

                    {
                        std::unique_lock lck(mut);
                        requestDone = true;
                    }
                    requestDoneCondition.notify_one();
                })
            .execute();

        // Wait for the request to finish
        std::unique_lock lck(mut);
        requestDoneCondition.wait(lck, [&requestDone] {
            return requestDone;
        });

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
        std::mutex mut;
        bool requestDone = false;
        std::condition_variable requestDoneCondition;

        NetworkRequest(url)
            .onSuccess([code, &mut, &requestDone, &requestDoneCondition,
                        url](NetworkResult result) -> Outcome {
                // The codes should throw an error
                EXPECT_TRUE(false);

                {
                    std::unique_lock lck(mut);
                    requestDone = true;
                }
                requestDoneCondition.notify_one();
                return Success;
            })
            .onError([code, &mut, &requestDone, &requestDoneCondition,
                      url](NetworkResult result) {
                EXPECT_EQ(result.status(), code);

                {
                    std::unique_lock lck(mut);
                    requestDone = true;
                }
                requestDoneCondition.notify_one();
            })
            .execute();

        // Wait for the request to finish
        std::unique_lock lck(mut);
        requestDoneCondition.wait(lck, [&requestDone] {
            return requestDone;
        });
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
        std::mutex mut;
        bool requestDone = false;
        std::condition_variable requestDoneCondition;

        bool finallyCalled = false;

        NetworkRequest(url)
            .finally(
                [&mut, &requestDone, &requestDoneCondition, &finallyCalled] {
                    finallyCalled = true;

                    {
                        std::unique_lock lck(mut);
                        requestDone = true;
                    }
                    requestDoneCondition.notify_one();
                })
            .execute();

        // Wait for the request to finish
        std::unique_lock lck(mut);
        requestDoneCondition.wait(lck, [&requestDone] {
            return requestDone;
        });

        EXPECT_TRUE(finallyCalled);
    }
}

TEST(NetworkRequest, TimeoutTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    auto url = getDelayURL(5);

    std::mutex mut;
    bool requestDone = false;
    std::condition_variable requestDoneCondition;

    NetworkRequest(url)
        .timeout(1000)
        .onSuccess([&mut, &requestDone, &requestDoneCondition,
                    url](NetworkResult result) -> Outcome {
            // The timeout should throw an error
            EXPECT_TRUE(false);

            {
                std::unique_lock lck(mut);
                requestDone = true;
            }
            requestDoneCondition.notify_one();
            return Success;
        })
        .onError([&mut, &requestDone, &requestDoneCondition,
                  url](NetworkResult result) {
            qDebug() << QTime::currentTime().toString()
                     << "timeout request finish error";
            EXPECT_EQ(result.status(), NetworkResult::timedoutStatus);

            {
                std::unique_lock lck(mut);
                requestDone = true;
            }
            requestDoneCondition.notify_one();
        })
        .execute();

    // Wait for the request to finish
    std::unique_lock lck(mut);
    requestDoneCondition.wait(lck, [&requestDone] {
        return requestDone;
    });

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, TimeoutNotTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    auto url = getDelayURL(1);

    std::mutex mut;
    bool requestDone = false;
    std::condition_variable requestDoneCondition;

    NetworkRequest(url)
        .timeout(2000)
        .onSuccess([&mut, &requestDone, &requestDoneCondition,
                    url](NetworkResult result) -> Outcome {
            EXPECT_EQ(result.status(), 200);

            {
                std::unique_lock lck(mut);
                requestDone = true;
            }
            requestDoneCondition.notify_one();
            return Success;
        })
        .onError([&mut, &requestDone, &requestDoneCondition,
                  url](NetworkResult result) {
            // The timeout should *not* throw an error
            EXPECT_TRUE(false);

            {
                std::unique_lock lck(mut);
                requestDone = true;
            }
            requestDoneCondition.notify_one();
        })
        .execute();

    // Wait for the request to finish
    std::unique_lock lck(mut);
    requestDoneCondition.wait(lck, [&requestDone] {
        return requestDone;
    });

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnTimeout)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    auto url = getDelayURL(5);

    std::mutex mut;
    bool requestDone = false;
    std::condition_variable requestDoneCondition;
    bool finallyCalled = false;
    bool onSuccessCalled = false;
    bool onErrorCalled = false;

    NetworkRequest(url)
        .timeout(1000)
        .onSuccess([&](NetworkResult result) -> Outcome {
            onSuccessCalled = true;
            return Success;
        })
        .onError([&](NetworkResult result) {
            onErrorCalled = true;
            EXPECT_EQ(result.status(), NetworkResult::timedoutStatus);
        })
        .finally([&] {
            finallyCalled = true;

            {
                std::unique_lock lck(mut);
                requestDone = true;
            }
            requestDoneCondition.notify_one();
        })
        .execute();

    // Wait for the request to finish
    std::unique_lock lck(mut);
    requestDoneCondition.wait(lck, [&requestDone] {
        return requestDone;
    });

    EXPECT_TRUE(finallyCalled);
    EXPECT_TRUE(onErrorCalled);
    EXPECT_FALSE(onSuccessCalled);
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}
