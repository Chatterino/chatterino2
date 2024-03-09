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
QString streamlink(const CommandContext &ctx);
QString popout(const CommandContext &ctx);
QString popup(const CommandContext &ctx);
QString clearmessages(const CommandContext &ctx);
QString openURL(const CommandContext &ctx);
QString sendRawMessage(const CommandContext &ctx);
QString injectFakeMessage(const CommandContext &ctx);
QString injectStreamUpdateNoStream(const CommandContext &ctx);
QString copyToClipboard(const CommandContext &ctx);
QString unstableSetUserClientSideColor(const CommandContext &ctx);
QString openUsercard(const CommandContext &ctx);

}  // namespace chatterino::commands
