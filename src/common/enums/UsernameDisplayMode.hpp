// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace chatterino {

enum UsernameDisplayMode : int {
    /// The user's display name, disregarding the localized name
    /// e.g. "forsen", "testaccount_420", "NymN"
    Username = 1,

    /// The user's display name or localized name
    /// e.g. "forsen", "테스트계정420", "NymN"
    LocalizedName = 2,

    /// The user's display name and, if it exists, their localized name
    /// e.g. "forsen", "testaccount_420 (테스트계정420)", "NymN"
    UsernameAndLocalizedName = 3,
};

}  // namespace chatterino
