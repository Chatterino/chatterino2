#include "singletons/Emotes.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"

namespace chatterino {

void Emotes::initialize(Application &app)
{
    const auto refreshTwitchEmotes = [this, &app] {
        auto currentUser = app.accounts->twitch.getCurrent();
        assert(currentUser);
        this->twitch.refresh(currentUser);
    };
    app.accounts->twitch.currentUserChanged.connect(refreshTwitchEmotes);
    refreshTwitchEmotes();

    this->emojis.load();
    this->bttv.loadGlobalEmotes();
    this->ffz.loadGlobalEmotes();

    this->gifTimer.initialize();
}

bool Emotes::isIgnoredEmote(const QString &)
{
    return false;
}

}  // namespace chatterino
