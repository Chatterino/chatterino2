#include "controllers/emotes/EmoteChannel.hpp"

namespace chatterino {

EmoteChannel::EmoteChannel(const QString &name, Type type)
    : Channel(name, type)
    , holder(this)
{
}

EmoteHolder &EmoteChannel::emotes()
{
    return this->holder;
}

const EmoteHolder &EmoteChannel::emotes() const
{
    return this->holder;
}

}  // namespace chatterino
