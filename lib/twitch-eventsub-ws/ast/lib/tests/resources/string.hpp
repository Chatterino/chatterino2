// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include <optional>
#include <string>
#include <vector>

struct String {
    std::string a;
    const std::string b;
    std::vector<std::string> c;
    std::optional<std::string> d;
};
