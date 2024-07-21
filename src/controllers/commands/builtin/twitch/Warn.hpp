#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /warn
QString sendWarn(const CommandContext &ctx);

}  // namespace chatterino::commands
