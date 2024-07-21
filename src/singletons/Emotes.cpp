#include "singletons/Emotes.hpp"

namespace chatterino {

Emotes::Emotes()
{
    this->emojis.load();

    this->gifTimer.initialize();
}

}  // namespace chatterino
