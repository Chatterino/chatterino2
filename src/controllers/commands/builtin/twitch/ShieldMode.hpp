#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString shieldModeOn(const CommandContext &ctx);
QString shieldModeOff(const CommandContext &ctx);

}  // namespace chatterino::commands
