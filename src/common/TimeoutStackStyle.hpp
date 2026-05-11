// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

namespace chatterino {

enum class TimeoutStackStyle : int {
    StackHard = 0,
    DontStackBeyondUserMessage = 1,
    DontStack = 2,

    Default = DontStackBeyondUserMessage,
};

}  // namespace chatterino
