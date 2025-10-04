#include "controllers/emotes/EmoteController.hpp"

#include "providers/emoji/Emojis.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "singletons/helper/GifTimer.hpp"

namespace chatterino {

EmoteController::EmoteController()
    : twitchEmotes_(std::make_unique<TwitchEmotes>())
    , emojis_(std::make_unique<Emojis>())
    , gifTimer_(std::make_unique<GIFTimer>())
{
}
EmoteController::~EmoteController() = default;

void EmoteController::initialize()
{
    this->emojis_->load();
    this->gifTimer_->initialize();
}

TwitchEmotes *EmoteController::getTwitchEmotes() const
{
    return this->twitchEmotes_.get();
}

Emojis *EmoteController::getEmojis() const
{
    return this->emojis_.get();
}

GIFTimer *EmoteController::getGIFTimer() const
{
    return this->gifTimer_.get();
}

}  // namespace chatterino
