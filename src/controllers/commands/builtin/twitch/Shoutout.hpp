#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString sendShoutout(const CommandContext &ctx);

}  // namespace chatterino::commands
