#include "IrcChannel2.hpp"

namespace chatterino {

IrcChannel::IrcChannel(const QString &name)
    : Channel(name, Channel::Type::Irc)
{
}

}  // namespace chatterino
