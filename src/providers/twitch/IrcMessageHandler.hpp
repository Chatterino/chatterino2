#pragma once

#include "messages/LimitedQueueSnapshot.hpp"

#include <IrcMessage>

#include <optional>
#include <vector>

namespace chatterino {

class ITwitchIrcServer;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;
class TwitchChannel;
class TwitchMessageBuilder;
class MessageSink;

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
    static void parseMessageInto(Communi::IrcMessage *message,
                                 MessageSink &sink, TwitchChannel *channel);

    void handlePrivMessage(Communi::IrcPrivateMessage *message,
                           ITwitchIrcServer &twitchServer);
    static void parsePrivMessageInto(Communi::IrcPrivateMessage *message,
                                     MessageSink &sink, TwitchChannel *channel);

    void handleRoomStateMessage(Communi::IrcMessage *message);
    void handleClearChatMessage(Communi::IrcMessage *message);
    void handleClearMessageMessage(Communi::IrcMessage *message);
    void handleUserStateMessage(Communi::IrcMessage *message);

    void handleWhisperMessage(Communi::IrcMessage *ircMessage);
    void handleUserNoticeMessage(Communi::IrcMessage *message,
                                 ITwitchIrcServer &twitchServer);
    static void parseUserNoticeMessageInto(Communi::IrcMessage *message,
                                           MessageSink &sink,
                                           TwitchChannel *channel);

    void handleNoticeMessage(Communi::IrcNoticeMessage *message);

    void handleJoinMessage(Communi::IrcMessage *message);
    void handlePartMessage(Communi::IrcMessage *message);

    static void addMessage(Communi::IrcMessage *message, MessageSink &sink,
                           TwitchChannel *chan, const QString &originalContent,
                           ITwitchIrcServer &twitch, bool isSub, bool isAction);

private:
    static float similarity(const MessagePtr &msg,
                            const LimitedQueueSnapshot<MessagePtr> &messages);
    static void setSimilarityFlags(const MessagePtr &message,
                                   const ChannelPtr &channel);
};

}  // namespace chatterino
