#pragma once

#include "common/Outcome.hpp"

namespace chatterino {

class IrcChannel;

Outcome invokeIrcCommand(const QString &command, const QString &params,
                         IrcChannel &channel);

}  // namespace chatterino
