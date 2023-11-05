#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /raid
QString startRaid(const CommandContext &ctx);

}  // namespace chatterino::commands
