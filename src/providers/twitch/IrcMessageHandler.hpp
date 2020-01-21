#pragma once

#include <IrcMessage>
#include "common/Channel.hpp"
#include "messages/Message.hpp"

namespace chatterino {

class TwitchIrcServer;
class Channel;

class IrcMessageHandler
{
    IrcMessageHandler() = default;

public:
    static IrcMessageHandler &instance();

    // parseMessage parses a single IRC message into 0+ Chatterino messages
    std::vector<MessagePtr> parseMessage(Channel *channel,
                                         Communi::IrcMessage *message);

    // parsePrivMessage arses a single IRC PRIVMSG into 0-1 Chatterino messages
    std::vector<MessagePtr> parsePrivMessage(
        Channel *channel, Communi::IrcPrivateMessage *message);
    void handlePrivMessage(Communi::IrcPrivateMessage *message,
                           TwitchIrcServer &server);

    void handleRoomStateMessage(Communi::IrcMessage *message);
    void handleClearChatMessage(Communi::IrcMessage *message);
    void handleClearMessageMessage(Communi::IrcMessage *message);
    void handleUserStateMessage(Communi::IrcMessage *message);
    void handleWhisperMessage(Communi::IrcMessage *message);

    // parseUserNoticeMessage parses a single IRC USERNOTICE message into 0+
    // chatterino messages
    std::vector<MessagePtr> parseUserNoticeMessage(
        Channel *channel, Communi::IrcMessage *message);
    void handleUserNoticeMessage(Communi::IrcMessage *message,
                                 TwitchIrcServer &server);

    void handleModeMessage(Communi::IrcMessage *message);

    // parseNoticeMessage parses a single IRC NOTICE message into 0+ chatterino
    // messages
    std::vector<MessagePtr> parseNoticeMessage(
        Communi::IrcNoticeMessage *message);
    void handleNoticeMessage(Communi::IrcNoticeMessage *message);

    void handleJoinMessage(Communi::IrcMessage *message);
    void handlePartMessage(Communi::IrcMessage *message);

    static float similarity(MessagePtr msg,
                            const LimitedQueueSnapshot<MessagePtr> &messages);

private:
    void addMessage(Communi::IrcMessage *message, const QString &target,
                    const QString &content, TwitchIrcServer &server,
                    bool isResub, bool isAction);
};

}  // namespace chatterino
