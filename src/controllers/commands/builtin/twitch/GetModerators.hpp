#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /mods
QString getModerators(const CommandContext &ctx);

}  // namespace chatterino::commands
