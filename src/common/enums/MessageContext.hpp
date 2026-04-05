// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace chatterino {

/** @exposeenum c2.MessageContext */

/// Context of the message being added to a channel
enum class MessageContext {
    /// This message is the original
    Original,
    /// This message is a repost of a message that has already been added in a channel
    Repost,
};

}  // namespace chatterino
