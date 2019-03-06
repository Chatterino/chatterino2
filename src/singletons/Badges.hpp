#pragma once

#include <unordered_map>
#include "common/Singleton.hpp"
#include "messages/Emote.hpp"

namespace chatterino
{
    class Badges : public Singleton
    {
    public:
        Badges();
    };

}  // namespace chatterino
