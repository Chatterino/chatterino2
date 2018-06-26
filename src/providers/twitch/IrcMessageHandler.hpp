#pragma once

#include <IrcMessage>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchServer;

class IrcMessageHandler
{
    IrcMessageHandler() = default;

public:
    static IrcMessageHandler &getInstance();

    void handlePrivMessage(Communi::IrcPrivateMessage *message, TwitchServer &server);

    void handleRoomStateMessage(Communi::IrcMessage *message);
    void handleClearChatMessage(Communi::IrcMessage *message);
    void handleUserStateMessage(Communi::IrcMessage *message);
    void handleWhisperMessage(Communi::IrcMessage *message);
    void handleUserNoticeMessage(Communi::IrcMessage *message, TwitchServer &server);
    void handleModeMessage(Communi::IrcMessage *message);
    void handleNoticeMessage(Communi::IrcNoticeMessage *message);
    void handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message);

    void handleJoinMessage(Communi::IrcMessage *message);
    void handlePartMessage(Communi::IrcMessage *message);

private:
    void addMessage(Communi::IrcMessage *message, const QString &target, const QString &content,
                    TwitchServer &server, bool isResub, bool isAction);
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
