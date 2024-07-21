#include "singletons/Emotes.hpp"

namespace chatterino {

Emotes::Emotes()
{
    this->emojis.load();

    this->gifTimer.initialize();
}

bool Emotes::isIgnoredEmote(const QString &)
{
    return false;
}

}  // namespace chatterino
