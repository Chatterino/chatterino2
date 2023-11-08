#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString sendWhisper(const CommandContext &ctx);

}  // namespace chatterino::commands
