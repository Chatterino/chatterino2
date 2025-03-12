#include "twitch-eventsub-ws/payloads/channel-ban-v1.hpp"

namespace chatterino::eventsub::lib::payload::channel_ban::v1 {

std::chrono::system_clock::duration Event::timeoutDuration() const
{
    if (!this->endsAt)
    {
        return {};
    }

    return *this->endsAt - this->bannedAt;
}

}  // namespace chatterino::eventsub::lib::payload::channel_ban::v1
