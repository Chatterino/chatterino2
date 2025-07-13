#include "twitch-eventsub-ws/payloads/channel-moderate-v2.hpp"

namespace chatterino::eventsub::lib::payload::channel_moderate::v2 {

bool Event::isFromSharedChat() const noexcept
{
    return this->sourceBroadcasterUserID && this->sourceBroadcasterUserLogin &&
           this->sourceBroadcasterUserName &&
           *this->sourceBroadcasterUserID != this->broadcasterUserID;
}
}  // namespace chatterino::eventsub::lib::payload::channel_moderate::v2
