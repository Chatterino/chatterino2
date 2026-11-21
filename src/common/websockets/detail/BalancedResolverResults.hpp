// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <boost/asio/ip/basic_resolver_results.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <optional>
#include <vector>

namespace chatterino::ws::detail {

/// An iterator over resolver results ensuring IPv4 and IPv6 entries are
/// balanced when iterating where possible.
///
/// If enough records are available, an IPv4 record is followed by an IPv6 one
/// and vice versa. The resolver starts with IPv4 entries.
class BalancedResolverResults
{
public:
    using Protocol = boost::asio::ip::tcp;
    using Entry = boost::asio::ip::basic_resolver_entry<Protocol>;

    BalancedResolverResults() = default;
    explicit BalancedResolverResults(
        const Protocol::resolver::results_type &results);
    explicit BalancedResolverResults(std::vector<Entry> entries);

    /// Advance to the next entry and return that one.
    ///
    /// Once the end is reached, `std::nullopt` is returned.
    std::optional<Entry> advanceEntry();

    /// Get the current entry.
    ///
    /// This only returns an entry after `advanceEntry` has been called.
    std::optional<Entry> currentEntry() const;

    /// Reset the iteration state.
    void reset();

private:
    /// Advance the IPv4 iterator.
    /// Returns true if `currentIdx` now points to an IPv4 entry.
    bool advanceIPv4();

    /// Advance the IPv6 iterator.
    /// Returns true if `currentIdx` now points to an IPv6 entry.
    bool advanceIPv6();

    size_t nextIPv4Idx = 0;
    size_t nextIPv6Idx = 0;
    bool nextIsIPv6 = true;

    size_t currentIdx = std::numeric_limits<size_t>::max();

    std::vector<Entry> entries;
};

}  // namespace chatterino::ws::detail
