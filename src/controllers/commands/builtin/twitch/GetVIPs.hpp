#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /vips
QString getVIPs(const CommandContext &ctx);

}  // namespace chatterino::commands
