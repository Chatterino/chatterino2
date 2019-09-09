#pragma once

#include "common/Channel.hpp"

namespace chatterino {

class IrcChannel : public Channel
{
public:
    explicit IrcChannel(const QString &name);
};

}  // namespace chatterino
