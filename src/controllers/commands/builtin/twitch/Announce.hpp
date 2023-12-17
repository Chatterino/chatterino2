#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /announce
QString sendAnnouncement(const CommandContext &ctx);

}  // namespace chatterino::commands
