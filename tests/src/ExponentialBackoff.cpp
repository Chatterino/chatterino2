#include "util/ExponentialBackoff.hpp"

#include "Test.hpp"

using namespace chatterino;

TEST(ExponentialBackoff, MaxSteps)
{
    using namespace std::literals::chrono_literals;

    ExponentialBackoff<3> foo{10ms};

    // First usage should be the start value
    EXPECT_EQ(foo.next(), 10ms);
    EXPECT_EQ(foo.next(), 20ms);
    EXPECT_EQ(foo.next(), 40ms);
    // We reached the max steps, so we should continue returning the max value without increasing
    EXPECT_EQ(foo.next(), 40ms);
    EXPECT_EQ(foo.next(), 40ms);
    EXPECT_EQ(foo.next(), 40ms);
}

TEST(ExponentialBackoff, Reset)
{
    using namespace std::literals::chrono_literals;

    ExponentialBackoff<3> foo{10ms};

    // First usage should be the start value
    EXPECT_EQ(foo.next(), 10ms);
    EXPECT_EQ(foo.next(), 20ms);
    EXPECT_EQ(foo.next(), 40ms);
    // We reached the max steps, so we should continue returning the max value without increasing
    EXPECT_EQ(foo.next(), 40ms);
    EXPECT_EQ(foo.next(), 40ms);
    EXPECT_EQ(foo.next(), 40ms);

    foo.reset();

    // After a reset, we should start at the beginning value again
    EXPECT_EQ(foo.next(), 10ms);
    EXPECT_EQ(foo.next(), 20ms);
    EXPECT_EQ(foo.next(), 40ms);
    // We reached the max steps, so we should continue returning the max value without increasing
    EXPECT_EQ(foo.next(), 40ms);
    EXPECT_EQ(foo.next(), 40ms);
    EXPECT_EQ(foo.next(), 40ms);
}

TEST(ExponentialBackoff, BadMaxSteps)
{
    using namespace std::literals::chrono_literals;

    // this will not compile
    // ExponentialBackoff<1> foo{10ms};
    // ExponentialBackoff<0> foo{10ms};
    // ExponentialBackoff<-1> foo{10ms};
}
