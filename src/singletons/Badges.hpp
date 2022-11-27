#pragma once

#include "common/Singleton.hpp"
#include "messages/Emote.hpp"

#include <unordered_map>

namespace chatterino {

class Badges : public Singleton
{
public:
    Badges();
};

}  // namespace chatterino
