#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /ban
QString sendBan(const CommandContext &ctx);
/// /banid
QString sendBanById(const CommandContext &ctx);

/// /timeout
QString sendTimeout(const CommandContext &ctx);

}  // namespace chatterino::commands
