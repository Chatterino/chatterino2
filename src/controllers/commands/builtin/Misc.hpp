#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString follow(const CommandContext &ctx);
QString unfollow(const CommandContext &ctx);
QString uptime(const CommandContext &ctx);

}  // namespace chatterino::commands
