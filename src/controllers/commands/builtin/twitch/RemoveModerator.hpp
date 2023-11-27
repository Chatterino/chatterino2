#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /unmod
QString removeModerator(const CommandContext &ctx);

}  // namespace chatterino::commands
