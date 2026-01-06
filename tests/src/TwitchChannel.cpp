// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/TwitchChannel.hpp"

#include "Test.hpp"

#include <QString>

#include <vector>

namespace chatterino::detail {

TEST(TwitchChannelDetail_isUnknownCommand, good)
{
    // clang-format off
    std::vector<QString> cases{
        "/me hello",
        ".me hello",
        "/ hello",
        ". hello",
        "/ /hello",
        ". .hello",
        "/ .hello",
        ". /hello",
        ".", // this results in an empty message but not in an error (twitchdev/issues#1019)
        "/me",
        ".me",
        "..",
        "...",
        "....",
        "",
        "foo",
        "a",
        "!",
        ". .",
        ". ..",
        ".. ..",
        ".. .",
        "/ /",
        "/ .",
        ". /",
        ". ./",
        ".. /",
        ".. me",
        ". me",
    };
    // clang-format on

    for (const auto &input : cases)
    {
        ASSERT_FALSE(isUnknownCommand(input))
            << input << " should not be considered an unknown command";
    }
}

TEST(TwitchChannelDetail_isUnknownCommand, bad)
{
    // clang-format off
    std::vector<QString> cases{
        "/badcommand",
        ".badcommand",
        "/badcommand hello",
        ".badcommand hello",
        "/@badcommand hello",
        ".@badcommand hello",
        "/bann username ban reason",
        "/bann username",
        "//",
        "./",
        "./me",
        "./w",
        "/.",
        "/.me",
        "/.w",
        "/,me",
    };
    // clang-format on

    for (const auto &input : cases)
    {
        ASSERT_TRUE(isUnknownCommand(input))
            << input << " should be considered an unknown command";
    }
}

}  // namespace chatterino::detail
