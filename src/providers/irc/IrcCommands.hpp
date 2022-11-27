#pragma once

#include "common/Outcome.hpp"

#include <QString>

namespace chatterino {

class IrcChannel;

Outcome invokeIrcCommand(const QString &command, const QString &params,
                         IrcChannel &channel);

}  // namespace chatterino
