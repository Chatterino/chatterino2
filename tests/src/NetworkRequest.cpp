#include "common/NetworkRequest.hpp"

#include "common/NetworkManager.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"

#include <gtest/gtest.h>
// Using this over <QTest> to avoid depending on the Test component of Qt
// We're only interested in QTest::qWaitFor.
#include <QtCore/qtestsupport_core.h>

using namespace chatterino;

namespace {

#ifdef CHATTERINO_TEST_USE_PUBLIC_HTTPBIN
// Not using httpbin.org, since it can be really slow and cause timeouts.
// postman-echo has the same API. They only differ for the 402 response.
const char *const HTTPBIN_BASE_URL = "https://postman-echo.com";
constexpr const bool CUSTOM_402_RESPONSE = false;
#else
const char *const HTTPBIN_BASE_URL = "http://127.0.0.1:9051";
constexpr const bool CUSTOM_402_RESPONSE = true;
#endif

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
        bool requestDone = false;

        NetworkRequest(url)
            .onSuccess([code, &requestDone,
                        url](const NetworkResult &result) -> Outcome {
                EXPECT_EQ(result.status(), code);

                requestDone = true;
                return Success;
            })
            .onError([&](const NetworkResult & /*result*/) {
                // The codes should *not* throw an error
                EXPECT_TRUE(false);

                requestDone = true;
            })
            .execute();

        // Wait for the request to finish
        EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
            return requestDone;
        }));
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
        bool requestDone = false;

        bool finallyCalled = false;

        NetworkRequest(url)
            .finally([&requestDone, &finallyCalled] {
                finallyCalled = true;
                requestDone = true;
            })
            .execute();

        // Wait for the request to finish
        EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
            return requestDone;
        }));

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
        bool requestDone = false;

        NetworkRequest(url)
            .onSuccess([&requestDone,
                        url](const NetworkResult & /*result*/) -> Outcome {
                // The codes should throw an error
                EXPECT_TRUE(false);

                requestDone = true;
                return Success;
            })
            .onError([code, &requestDone, url](const NetworkResult &result) {
                EXPECT_EQ(result.status(), code);
                if (code == 402)
                {
                    if (CUSTOM_402_RESPONSE)
                    {
                        EXPECT_EQ(result.getData(),
                                  QByteArrayLiteral("Fuck you, pay me!"));
                    }
                    else
                    {
                        EXPECT_EQ(result.getData(),
                                  QByteArrayLiteral("{\n  \"status\": 402\n}"));
                    }
                }

                requestDone = true;
            })
            .execute();

        // Wait for the request to finish
        EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
            return requestDone;
        }));
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
        bool requestDone = false;

        bool finallyCalled = false;

        NetworkRequest(url)
            .finally([&requestDone, &finallyCalled] {
                finallyCalled = true;
                requestDone = true;
            })
            .execute();

        // Wait for the request to finish
        EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
            return requestDone;
        }));

        EXPECT_TRUE(finallyCalled);
    }
}

TEST(NetworkRequest, TimeoutTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    auto url = getDelayURL(5);
    bool requestDone = false;

    NetworkRequest(url)
        .timeout(1000)
        .onSuccess([&requestDone](const NetworkResult & /*result*/) -> Outcome {
            // The timeout should throw an error
            EXPECT_TRUE(false);
            requestDone = true;
            return Success;
        })
        .onError([&requestDone, url](const NetworkResult &result) {
            qDebug() << QTime::currentTime().toString()
                     << "timeout request finish error";
            EXPECT_EQ(result.status(), NetworkResult::timedoutStatus);

            requestDone = true;
        })
        .execute();

    // Wait for the request to finish
    EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
        return requestDone;
    }));

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, TimeoutNotTimingOut)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    auto url = getDelayURL(1);

    bool requestDone = false;

    NetworkRequest(url)
        .timeout(2000)
        .onSuccess([&requestDone, url](const NetworkResult &result) -> Outcome {
            EXPECT_EQ(result.status(), 200);

            requestDone = true;
            return Success;
        })
        .onError([&requestDone, url](const NetworkResult & /*result*/) {
            // The timeout should *not* throw an error
            EXPECT_TRUE(false);
            requestDone = true;
        })
        .execute();

    // Wait for the request to finish
    EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
        return requestDone;
    }));

    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}

TEST(NetworkRequest, FinallyCallbackOnTimeout)
{
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());

    auto url = getDelayURL(5);

    bool requestDone = false;
    bool finallyCalled = false;
    bool onSuccessCalled = false;
    bool onErrorCalled = false;

    NetworkRequest(url)
        .timeout(1000)
        .onSuccess([&](const NetworkResult & /*result*/) -> Outcome {
            onSuccessCalled = true;
            return Success;
        })
        .onError([&](const NetworkResult &result) {
            onErrorCalled = true;
            EXPECT_EQ(result.status(), NetworkResult::timedoutStatus);
        })
        .finally([&] {
            finallyCalled = true;
            requestDone = true;
        })
        .execute();

    // Wait for the request to finish
    EXPECT_TRUE(QTest::qWaitFor([&requestDone] {
        return requestDone;
    }));

    EXPECT_TRUE(finallyCalled);
    EXPECT_TRUE(onErrorCalled);
    EXPECT_FALSE(onSuccessCalled);
    EXPECT_TRUE(NetworkManager::workerThread.isRunning());
}
