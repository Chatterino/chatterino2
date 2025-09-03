#pragma once

#include "common/Channel.hpp"
#include "controllers/emotes/EmoteHolder.hpp"

namespace chatterino {

class EmoteChannel : public Channel
{
public:
    explicit EmoteChannel(const QString &name, Type type);

    EmoteHolder &emotes();
    const EmoteHolder &emotes() const;

private:
    EmoteHolder holder;
};

}  // namespace chatterino
