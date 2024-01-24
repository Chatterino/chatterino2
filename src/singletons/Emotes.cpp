#include "singletons/Emotes.hpp"

namespace chatterino {

Emotes::Emotes()
{
}

void Emotes::initialize(Settings &settings, const Paths &paths)
{
    this->emojis.load();

    this->gifTimer.initialize();
}

bool Emotes::isIgnoredEmote(const QString &)
{
    return false;
}

}  // namespace chatterino
