#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /unban
QString unbanUser(const CommandContext &ctx);

}  // namespace chatterino::commands
