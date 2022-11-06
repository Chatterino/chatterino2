#pragma once

#include "controllers/commands/CommandContext.hpp"

#include <QString>

namespace chatterino::commands {

QString emoteOnly(const CommandContext &ctx);
QString emoteOnlyOff(const CommandContext &ctx);

QString subscribers(const CommandContext &ctx);
QString subscribersOff(const CommandContext &ctx);

QString slow(const CommandContext &ctx);
QString slowOff(const CommandContext &ctx);

QString followers(const CommandContext &ctx);
QString followersOff(const CommandContext &ctx);

QString uniqueChat(const CommandContext &ctx);
QString uniqueChatOff(const CommandContext &ctx);

}  // namespace chatterino::commands
