#include "singletons/Emotes.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"

namespace chatterino {

Emotes::Emotes()
{
}

void Emotes::initialize(Settings &settings, Paths &paths)
{
    this->emojis.load();

    this->gifTimer.initialize();
}

bool Emotes::isIgnoredEmote(const QString &)
{
    return false;
}

}  // namespace chatterino
