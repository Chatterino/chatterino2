#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /poll
QString createPoll(const CommandContext &ctx);

/// /endpoll
QString endPoll(const CommandContext &ctx);

/// /cancelpoll
QString cancelPoll(const CommandContext &ctx);

}  // namespace chatterino::commands
