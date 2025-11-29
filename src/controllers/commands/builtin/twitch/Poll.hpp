#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /poll
QString createPoll(const CommandContext &ctx);

}  // namespace chatterino::commands
