#pragma once

#include <IrcMessage>

namespace chatterino {
namespace singletons {
class ChannelManager;
class ResourceManager;

namespace helper {
class IrcMessageHandler
{
    IrcMessageHandler(ChannelManager &channelManager, ResourceManager &resourceManager);

    ChannelManager &channelManager;
    ResourceManager &resourceManager;

public:
    static IrcMessageHandler &getInstance();

    void handleRoomStateMessage(Communi::IrcMessage *message);
    void handleClearChatMessage(Communi::IrcMessage *message);
    void handleUserStateMessage(Communi::IrcMessage *message);
    void handleWhisperMessage(Communi::IrcMessage *message);
    void handleUserNoticeMessage(Communi::IrcMessage *message);
    void handleModeMessage(Communi::IrcMessage *message);
    void handleNoticeMessage(Communi::IrcNoticeMessage *message);
    void handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message);
};

}  // namespace helper
}  // namespace singletons
}  // namespace chatterino
