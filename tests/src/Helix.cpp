// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/api/Helix.hpp"

#include "common/enums/UsernameDisplayMode.hpp"
#include "Test.hpp"

#include <QColor>

namespace chatterino {

TEST(HelixMinimalUser, Formatted)
{
    using UsernameDisplayMode::LocalizedName;
    using UsernameDisplayMode::Username;
    using UsernameDisplayMode::UsernameAndLocalizedName;

    // clang-format off

    HelixMinimalUser forsen{
        .id = "22484632",
        .login = "forsen",
        .displayName = "forsen",
    };
    EXPECT_EQ(forsen.formatted(Username), "forsen");
    EXPECT_EQ(forsen.formatted(LocalizedName), "forsen");
    EXPECT_EQ(forsen.formatted(UsernameAndLocalizedName), "forsen");

    HelixMinimalUser ta420{
        .id = "117166826",
        .login = "testaccount_420",
        .displayName = "테스트계정420",
    };
    EXPECT_EQ(ta420.formatted(Username), "testaccount_420");
    EXPECT_EQ(ta420.formatted(LocalizedName), "테스트계정420");
    EXPECT_EQ(ta420.formatted(UsernameAndLocalizedName), "testaccount_420 (테스트계정420)");

    HelixMinimalUser nymn{
        .id = "62300805",
        .login = "nymn",
        .displayName = "NymN",
    };
    EXPECT_EQ(nymn.formatted(Username), "NymN");
    EXPECT_EQ(nymn.formatted(LocalizedName), "NymN");
    EXPECT_EQ(nymn.formatted(UsernameAndLocalizedName), "NymN");

    // clang-format on
}

}  // namespace chatterino
