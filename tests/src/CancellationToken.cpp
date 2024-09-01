#include "util/CancellationToken.hpp"

#include <gtest/gtest.h>

using namespace chatterino;

// CancellationToken

TEST(CancellationToken, ctor)
{
    {
        CancellationToken token;
        ASSERT_TRUE(token.isCancelled());
        token.cancel();
        ASSERT_TRUE(token.isCancelled());
    }
    {
        CancellationToken token(false);
        ASSERT_FALSE(token.isCancelled());
        token.cancel();
        ASSERT_TRUE(token.isCancelled());
    }
    {
        CancellationToken token(true);
        ASSERT_TRUE(token.isCancelled());
        token.cancel();
        ASSERT_TRUE(token.isCancelled());
    }
}

TEST(CancellationToken, moveCtor)
{
    CancellationToken token(false);
    ASSERT_FALSE(token.isCancelled());
    CancellationToken token2(std::move(token));
    // NOLINTNEXTLINE(bugprone-use-after-move)
    ASSERT_TRUE(token.isCancelled());
    ASSERT_FALSE(token2.isCancelled());

    token.cancel();
    ASSERT_FALSE(token2.isCancelled());

    token2.cancel();
    ASSERT_TRUE(token2.isCancelled());
}

TEST(CancellationToken, moveAssign)
{
    CancellationToken token(false);
    CancellationToken token2;
    ASSERT_FALSE(token.isCancelled());
    ASSERT_TRUE(token2.isCancelled());

    token2 = std::move(token);
    // NOLINTNEXTLINE(bugprone-use-after-move)
    ASSERT_TRUE(token.isCancelled());
    ASSERT_FALSE(token2.isCancelled());
    token.cancel();
    ASSERT_FALSE(token2.isCancelled());

    token2.cancel();
    ASSERT_TRUE(token2.isCancelled());
}

TEST(CancellationToken, copyCtor)
{
    CancellationToken token(false);
    ASSERT_FALSE(token.isCancelled());
    CancellationToken token2(token);
    ASSERT_FALSE(token.isCancelled());
    ASSERT_FALSE(token2.isCancelled());
    token2.cancel();
    ASSERT_TRUE(token2.isCancelled());
    ASSERT_TRUE(token.isCancelled());
}

TEST(CancellationToken, copyAssign)
{
    CancellationToken token(false);
    CancellationToken token2;
    ASSERT_FALSE(token.isCancelled());
    ASSERT_TRUE(token2.isCancelled());

    token2 = token;
    ASSERT_FALSE(token.isCancelled());
    ASSERT_FALSE(token2.isCancelled());
    token2.cancel();
    ASSERT_TRUE(token.isCancelled());
    ASSERT_TRUE(token2.isCancelled());
}

TEST(CancellationToken, dtor)
{
    CancellationToken token(false);
    {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        CancellationToken token2 = token;
        ASSERT_FALSE(token.isCancelled());
        ASSERT_FALSE(token2.isCancelled());
    }

    ASSERT_FALSE(token.isCancelled());
}

// ScopedCancellationToken

TEST(ScopedCancellationToken, moveCancelCtor)
{
    CancellationToken token(false);
    ASSERT_FALSE(token.isCancelled());
    {
        ScopedCancellationToken scoped(std::move(token));
        // NOLINTNEXTLINE(bugprone-use-after-move)
        ASSERT_TRUE(token.isCancelled());
    }
}

TEST(ScopedCancellationToken, copyCancelCtor)
{
    CancellationToken token(false);
    ASSERT_FALSE(token.isCancelled());
    {
        ScopedCancellationToken scoped(token);
        ASSERT_FALSE(token.isCancelled());
    }
    ASSERT_TRUE(token.isCancelled());
}

TEST(ScopedCancellationToken, moveCtor)
{
    CancellationToken token(false);
    ASSERT_FALSE(token.isCancelled());
    {
        ScopedCancellationToken scoped(token);
        {
            ScopedCancellationToken inner(std::move(scoped));
            ASSERT_FALSE(token.isCancelled());
        }
        ASSERT_TRUE(token.isCancelled());
    }
}

TEST(ScopedCancellationToken, moveAssign)
{
    CancellationToken token(false);
    CancellationToken token2(false);
    {
        ScopedCancellationToken scoped(token);
        {
            ScopedCancellationToken inner(token2);
            ASSERT_FALSE(token.isCancelled());
            ASSERT_FALSE(token2.isCancelled());
            inner = std::move(scoped);
            ASSERT_FALSE(token.isCancelled());
            ASSERT_TRUE(token2.isCancelled());
        }
        ASSERT_TRUE(token.isCancelled());
    }
}

TEST(ScopedCancellationToken, copyAssign)
{
    CancellationToken token(false);
    CancellationToken token2(false);
    {
        ScopedCancellationToken scoped(token);
        ASSERT_FALSE(token.isCancelled());
        scoped = token2;
        ASSERT_FALSE(token2.isCancelled());
        ASSERT_TRUE(token.isCancelled());
    }
    ASSERT_TRUE(token2.isCancelled());
}
