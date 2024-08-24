#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString setLoggingRules(const CommandContext &ctx);

QString toggleThemeReload(const CommandContext &ctx);

QString listEnvironmentVariables(const CommandContext &ctx);

QString listArgs(const CommandContext &ctx);

QString forceImageGarbageCollection(const CommandContext &ctx);

QString forceImageUnload(const CommandContext &ctx);

QString debugTest(const CommandContext &ctx);

}  // namespace chatterino::commands
