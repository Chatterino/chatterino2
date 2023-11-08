#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString updateUserColor(const CommandContext &ctx);

}  // namespace chatterino::commands
