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

}  // namespace chatterino::commands
