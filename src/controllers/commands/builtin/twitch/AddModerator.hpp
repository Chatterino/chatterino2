#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /mod
QString addModerator(const CommandContext &ctx);

}  // namespace chatterino::commands
