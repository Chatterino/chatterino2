#include "singletons/Emotes.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"

namespace chatterino {

Emotes::Emotes()
{
}

void Emotes::initialize(Settings &settings, Paths &paths)
{
    getApp()->accounts->twitch.currentUserChanged.connect(
        [] { getApp()->accounts->twitch.getCurrent()->loadEmotes(); });

    this->emojis.load();

    this->gifTimer.initialize();
}

bool Emotes::isIgnoredEmote(const QString &)
{
    return false;
}

}  // namespace chatterino
