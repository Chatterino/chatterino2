#include "singletons/emotemanager.hpp"

#include "application.hpp"
#include "controllers/accounts/accountcontroller.hpp"

namespace chatterino {
namespace singletons {

void EmoteManager::initialize()
{
    getApp()->accounts->twitch.currentUserChanged.connect([this] {
        auto currentUser = getApp()->accounts->twitch.getCurrent();
        assert(currentUser);
        this->twitch.refresh(currentUser);
    });

    this->emojis.load();
    this->bttv.loadGlobalEmotes();
    this->ffz.loadGlobalEmotes();

    this->gifTimer.initialize();
}

}  // namespace singletons
}  // namespace chatterino
