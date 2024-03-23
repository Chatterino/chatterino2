#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /block
QString blockUser(const CommandContext &ctx);

/// /ignore
QString ignoreUser(const CommandContext &ctx);

/// /unblock
QString unblockUser(const CommandContext &ctx);

/// /unignore
QString unignoreUser(const CommandContext &ctx);

}  // namespace chatterino::commands
