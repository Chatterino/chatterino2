// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /raid
QString startRaid(const CommandContext &ctx);

/// /unraid
QString cancelRaid(const CommandContext &ctx);

}  // namespace chatterino::commands
