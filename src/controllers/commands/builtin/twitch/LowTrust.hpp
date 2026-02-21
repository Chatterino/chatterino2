// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /monitor
QString monitorUser(const CommandContext &ctx);

/// /restrict
QString restrictUser(const CommandContext &ctx);

/// /unmonitor
QString unmonitorUser(const CommandContext &ctx);

/// /unrestrict
QString unrestrictUser(const CommandContext &ctx);

}  // namespace chatterino::commands
