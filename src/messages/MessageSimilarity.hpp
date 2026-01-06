// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/Message.hpp"

#include <ranges>
namespace chatterino {

template <std::ranges::bidirectional_range T>
void setSimilarityFlags(const MessagePtr &message, const T &messages);

}  // namespace chatterino
