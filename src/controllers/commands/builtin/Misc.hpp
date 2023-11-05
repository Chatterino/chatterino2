#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString follow(const CommandContext &ctx);
QString unfollow(const CommandContext &ctx);
QString uptime(const CommandContext &ctx);
QString user(const CommandContext &ctx);
QString requests(const CommandContext &ctx);
QString lowtrust(const CommandContext &ctx);
QString clip(const CommandContext &ctx);
QString marker(const CommandContext &ctx);

}  // namespace chatterino::commands
