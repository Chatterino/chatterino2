#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString chatters(const CommandContext &ctx);

QString testChatters(const CommandContext &ctx);

}  // namespace chatterino::commands
