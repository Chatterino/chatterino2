#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString setTitle(const CommandContext &ctx);
QString setGame(const CommandContext &ctx);

}  // namespace chatterino::commands
