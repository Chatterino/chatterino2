#include "common/network/NetworkResult.hpp"

#include "Test.hpp"

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
    checkResult({Error::InternalServerError, 400, {}},
                Error::InternalServerError, 400, "400");

    // error takes precedence (1..=99)
    checkResult({Error::BackgroundRequestNotAllowedError, 400, {}},
                Error::BackgroundRequestNotAllowedError, 400,
                "BackgroundRequestNotAllowedError");
    checkResult({Error::UnknownNetworkError, 400, {}},
                Error::UnknownNetworkError, 400, "UnknownNetworkError");
}

TEST(NetworkResult, InvalidError)
{
    checkResult({static_cast<Error>(-1), {}, {}}, static_cast<Error>(-1),
                std::nullopt, "unknown error (-1)");
    checkResult({static_cast<Error>(-1), 42, {}}, static_cast<Error>(-1), 42,
                "unknown error (status: 42, error: -1)");
}
