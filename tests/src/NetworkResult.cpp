#include "common/network/NetworkResult.hpp"

#include <gtest/gtest.h>

using namespace chatterino;

using Error = NetworkResult::NetworkError;

namespace {

void checkResult(const NetworkResult &res, Error error,
                 std::optional<int> status, const QString &formatted)
{
    ASSERT_EQ(res.error(), error);
    ASSERT_EQ(res.status(), status);
    ASSERT_EQ(res.formatError(), formatted);
}

}  // namespace

TEST(NetworkResult, NoError)
{
    checkResult({Error::NoError, 200, {}}, Error::NoError, 200, "200");
    checkResult({Error::NoError, 202, {}}, Error::NoError, 202, "202");

    // no status code
    checkResult({Error::NoError, {}, {}}, Error::NoError, std::nullopt,
                "NoError");
}

TEST(NetworkResult, Errors)
{
    checkResult({Error::TimeoutError, {}, {}}, Error::TimeoutError,
                std::nullopt, "TimeoutError");
    checkResult({Error::RemoteHostClosedError, {}, {}},
                Error::RemoteHostClosedError, std::nullopt,
                "RemoteHostClosedError");

    // status code takes precedence
    checkResult({Error::TimeoutError, 400, {}}, Error::TimeoutError, 400,
                "400");
}

TEST(NetworkResult, InvalidError)
{
    checkResult({static_cast<Error>(-1), {}, {}}, static_cast<Error>(-1),
                std::nullopt, "unknown error (-1)");
}
