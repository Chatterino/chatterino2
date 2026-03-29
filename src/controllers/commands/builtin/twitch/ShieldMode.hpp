// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString shieldModeOn(const CommandContext &ctx);
QString shieldModeOff(const CommandContext &ctx);

}  // namespace chatterino::commands
