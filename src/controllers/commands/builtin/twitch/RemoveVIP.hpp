#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /unvip
QString removeVIP(const CommandContext &ctx);

}  // namespace chatterino::commands
