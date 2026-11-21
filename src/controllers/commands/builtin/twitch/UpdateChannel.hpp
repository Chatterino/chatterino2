// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString setTitle(const CommandContext &ctx);
QString setGame(const CommandContext &ctx);

}  // namespace chatterino::commands
