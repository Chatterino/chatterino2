#include "messagefactory.hpp"

namespace chatterino {

MessageFactory::MessageFactory(Resources &_resources, EmoteManager &_emoteManager,
                               WindowManager &_windowManager)
    : resources(_resources)
    , emoteManager(_emoteManager)
    , windowManager(_windowManager)
{
}

messages::SharedMessage MessageFactory::buildMessage(Communi::IrcPrivateMessage *message,
                                                     Channel &channel,
                                                     const messages::MessageParseArgs &args)
{
    return nullptr;
}

}  // namespace chatterino
