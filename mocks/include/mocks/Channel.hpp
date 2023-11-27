#pragma once

#include "common/Channel.hpp"

namespace chatterino::mock {

class MockChannel : public Channel
{
public:
    MockChannel(const QString &name)
        : Channel(name, Channel::Type::Twitch)
    {
    }
};

}  // namespace chatterino::mock
