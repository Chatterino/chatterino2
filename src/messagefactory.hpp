#pragma once

#include "messages/message.hpp"

namespace chatterino {

class Resources;
class EmoteManager;
class WindowManager;

class MessageFactory
{
public:
    explicit MessageFactory(Resources &_resources, EmoteManager &_emoteManager,
                            WindowManager &_windowManager);

    messages::SharedMessage buildMessage(Communi::IrcPrivateMessage *message, Channel &channel,
                                         const messages::MessageParseArgs &args);

private:
    Resources &resources;
    EmoteManager &emoteManager;
    WindowManager &windowManager;
};

}  // namespace chatterino
