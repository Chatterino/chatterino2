#pragma once

#include <IrcMessage>

namespace chatterino
{
    class TwitchRoom;

    void handleMessage(Communi::IrcMessage* message, TwitchRoom* room);
    void handlePrivMsg(Communi::IrcPrivateMessage* message, TwitchRoom& room);
}  // namespace chatterino
