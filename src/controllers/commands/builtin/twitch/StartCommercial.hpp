#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /commercial
QString startCommercial(const CommandContext &ctx);

}  // namespace chatterino::commands
