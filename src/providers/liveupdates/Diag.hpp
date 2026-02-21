// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>

namespace chatterino::liveupdates {

struct Diag {
    std::atomic<uint32_t> connectionsClosed{0};
    std::atomic<uint32_t> connectionsOpened{0};
    std::atomic<uint32_t> connectionsFailed{0};
};

}  // namespace chatterino::liveupdates
