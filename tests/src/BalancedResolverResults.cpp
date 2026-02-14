// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/websockets/detail/BalancedResolverResults.hpp"

#include "Test.hpp"

#include <boost/asio/ip/tcp.hpp>

using namespace chatterino::ws::detail;

namespace {

using Entry = BalancedResolverResults::Entry;

void runCheck(BalancedResolverResults results,
              const std::vector<Entry> &expectedOrder)
{
    ASSERT_FALSE(expectedOrder.empty());

    for (size_t j = 0; j < 4; j++)
    {
        EXPECT_EQ(results.currentEntry(), std::nullopt);
        for (size_t i = 0; i < expectedOrder.size(); i++)
        {
            EXPECT_EQ(results.advanceEntry()->endpoint(),
                      expectedOrder[i].endpoint())
                << i;
            EXPECT_EQ(results.currentEntry()->endpoint(),
                      expectedOrder[i].endpoint())
                << i;
        }
        EXPECT_EQ(results.advanceEntry(), std::nullopt);
        EXPECT_EQ(results.currentEntry()->endpoint(),
                  expectedOrder.back().endpoint());

        results.reset();
    }
}

Entry makeEntry(const boost::asio::ip::address &addr)
{
    return {
        boost::asio::ip::tcp::endpoint{addr, 443},
        "host.example.com",
        "service",
    };
}

Entry makeV4(const char *spec)
{
    return makeEntry(boost::asio::ip::make_address_v4(spec));
}

Entry makeV6(const char *spec)
{
    return makeEntry(boost::asio::ip::make_address_v6(spec));
}

}  // namespace

TEST(BalancedResolverResults, empty)
{
    BalancedResolverResults results;
    ASSERT_EQ(results.currentEntry(), std::nullopt);
    ASSERT_EQ(results.advanceEntry(), std::nullopt);
    ASSERT_EQ(results.currentEntry(), std::nullopt);
    ASSERT_EQ(results.advanceEntry(), std::nullopt);
    ASSERT_EQ(results.currentEntry(), std::nullopt);
    ASSERT_EQ(results.advanceEntry(), std::nullopt);
    ASSERT_EQ(results.currentEntry(), std::nullopt);
    ASSERT_EQ(results.advanceEntry(), std::nullopt);
    results.reset();
    ASSERT_EQ(results.currentEntry(), std::nullopt);
    ASSERT_EQ(results.advanceEntry(), std::nullopt);
}

TEST(BalancedResolverResults, singleV4)
{
    std::vector<Entry> entries{makeV4("10.10.10.1")};
    runCheck(BalancedResolverResults(entries), entries);
}

TEST(BalancedResolverResults, singleV6)
{
    std::vector<Entry> entries{makeV6("1234:5678:9abc:def::1")};
    runCheck(BalancedResolverResults(entries), entries);
}

TEST(BalancedResolverResults, singleV4AndV6)
{
    std::vector<Entry> entries{
        makeV6("1234:5678:9abc:def::1"),
        makeV4("10.10.10.1"),
    };
    runCheck(BalancedResolverResults(entries), entries);
}

TEST(BalancedResolverResults, doubleV4)
{
    runCheck(BalancedResolverResults({
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
             }),
             {
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
             });
}

TEST(BalancedResolverResults, doubleV6)
{
    runCheck(BalancedResolverResults({
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
             });
}

TEST(BalancedResolverResults, doubleV4SingleV6)
{
    runCheck(BalancedResolverResults({
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
             });
}

TEST(BalancedResolverResults, singleV4DoubleV6)
{
    runCheck(BalancedResolverResults({
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV4("10.10.10.1"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV6("1234:5678:9abc:def::2"),
             });
}

TEST(BalancedResolverResults, doubleV4AndV6)
{
    runCheck(BalancedResolverResults({
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV4("10.10.10.2"),
             });
}

TEST(BalancedResolverResults, tripleV6)
{
    runCheck(BalancedResolverResults({
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV6("1234:5678:9abc:def::3"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV6("1234:5678:9abc:def::3"),
             });
}

TEST(BalancedResolverResults, tripleV4)
{
    runCheck(BalancedResolverResults({
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
                 makeV4("10.10.10.3"),
             }),
             {
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
                 makeV4("10.10.10.3"),
             });
}

TEST(BalancedResolverResults, tripleV4SingleV6)
{
    runCheck(BalancedResolverResults({
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.3"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
                 makeV4("10.10.10.3"),
             });
}

TEST(BalancedResolverResults, singleV4TripleV6)
{
    runCheck(BalancedResolverResults({
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV4("10.10.10.1"),
                 makeV6("1234:5678:9abc:def::3"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV6("1234:5678:9abc:def::3"),
             });
}

TEST(BalancedResolverResults, tripleV4AndV6)
{
    runCheck(BalancedResolverResults({
                 makeV4("10.10.10.1"),
                 makeV4("10.10.10.2"),
                 makeV4("10.10.10.3"),
                 makeV6("1234:5678:9abc:def::1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV6("1234:5678:9abc:def::3"),
             }),
             {
                 makeV6("1234:5678:9abc:def::1"),
                 makeV4("10.10.10.1"),
                 makeV6("1234:5678:9abc:def::2"),
                 makeV4("10.10.10.2"),
                 makeV6("1234:5678:9abc:def::3"),
                 makeV4("10.10.10.3"),
             });
}
