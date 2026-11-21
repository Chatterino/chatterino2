// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /pin
QString pin(const CommandContext &ctx);

/// /unpin
QString unpin(const CommandContext &ctx);

}  // namespace chatterino::commands
