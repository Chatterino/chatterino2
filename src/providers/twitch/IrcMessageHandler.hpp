#pragma once

#include "messages/LimitedQueueSnapshot.hpp"

#include <IrcMessage>

#include <optional>
#include <vector>

namespace chatterino {

class TwitchIrcServer;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;
class TwitchChannel;
class TwitchMessageBuilder;

struct ClearChatMessage {
    MessagePtr message;
    bool disableAllMessages;
};

class IrcMessageHandler
{
    IrcMessageHandler() = default;

public:
    static IrcMessageHandler &instance();

    /**
     * Parse an IRC message into 0 or more Chatterino messages
     * Takes previously loaded messages into consideration to add reply contexts
     **/
    static std::vector<MessagePtr> parseMessageWithReply(
        Channel *channel, Communi::IrcMessage *message,
        std::vector<MessagePtr> &otherLoaded);

    void handlePrivMessage(Communi::IrcPrivateMessage *message,
                           TwitchIrcServer &server);

    void handleRoomStateMessage(Communi::IrcMessage *message);
    void handleClearChatMessage(Communi::IrcMessage *message);
    void handleClearMessageMessage(Communi::IrcMessage *message);
    void handleUserStateMessage(Communi::IrcMessage *message);
    void handleGlobalUserStateMessage(Communi::IrcMessage *message);
    void handleWhisperMessage(Communi::IrcMessage *ircMessage);

    void handleUserNoticeMessage(Communi::IrcMessage *message,
                                 TwitchIrcServer &server);

    void handleNoticeMessage(Communi::IrcNoticeMessage *message);

    void handleJoinMessage(Communi::IrcMessage *message);
    void handlePartMessage(Communi::IrcMessage *message);

    void addMessage(Communi::IrcMessage *message, const ChannelPtr &chan,
                    const QString &originalContent, TwitchIrcServer &server,
                    bool isSub, bool isAction);

private:
    static float similarity(const MessagePtr &msg,
                            const LimitedQueueSnapshot<MessagePtr> &messages);
    static void setSimilarityFlags(const MessagePtr &message,
                                   const ChannelPtr &channel);
};

}  // namespace chatterino
