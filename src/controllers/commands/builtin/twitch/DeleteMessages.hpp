#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /clear
QString deleteAllMessages(const CommandContext &ctx);

/// /delete
QString deleteOneMessage(const CommandContext &ctx);

}  // namespace chatterino::commands
