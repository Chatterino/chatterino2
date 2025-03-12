#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/MessageColor.hpp"
#include "messages/MessageFlag.hpp"
#include "providers/twitch/pubsubmessages/LowTrustUsers.hpp"

#include <IrcMessage>
#include <QRegularExpression>
#include <QString>
#include <QTime>
#include <QVariant>

#include <ctime>
#include <memory>
#include <unordered_map>
#include <utility>

namespace chatterino {

struct BanAction;
struct UnbanAction;
struct WarnAction;
struct RaidAction;
struct UnraidAction;
struct AutomodAction;
struct AutomodUserAction;
struct AutomodInfoAction;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;
using MessagePtrMut = std::shared_ptr<Message>;

class MessageElement;
class TextElement;
struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Channel;
class TwitchChannel;
class MessageThread;
class IgnorePhrase;
struct HelixVip;
using HelixModerator = HelixVip;
struct ChannelPointReward;
struct DeleteAction;
struct TwitchEmoteOccurrence;

namespace linkparser {
    struct Parsed;
}  // namespace linkparser

struct SystemMessageTag {
};
struct TimeoutMessageTag {
};
struct LiveUpdatesUpdateEmoteMessageTag {
};
struct LiveUpdatesRemoveEmoteMessageTag {
};
struct LiveUpdatesAddEmoteMessageTag {
};
struct LiveUpdatesUpdateEmoteSetMessageTag {
};
struct ImageUploaderResultTag {
};

// NOLINTBEGIN(readability-identifier-naming)
const SystemMessageTag systemMessage{};
const TimeoutMessageTag timeoutMessage{};
const LiveUpdatesUpdateEmoteMessageTag liveUpdatesUpdateEmoteMessage{};
const LiveUpdatesRemoveEmoteMessageTag liveUpdatesRemoveEmoteMessage{};
const LiveUpdatesAddEmoteMessageTag liveUpdatesAddEmoteMessage{};
const LiveUpdatesUpdateEmoteSetMessageTag liveUpdatesUpdateEmoteSetMessage{};

// This signifies that you want to construct a message containing the result of
// a successful image upload.
const ImageUploaderResultTag imageUploaderResultMessage{};
// NOLINTEND(readability-identifier-naming)

MessagePtr makeSystemMessage(const QString &text);
MessagePtr makeSystemMessage(const QString &text, const QTime &time);

struct MessageParseArgs {
    bool disablePingSounds = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool trimSubscriberUsername = false;
    bool isStaffOrBroadcaster = false;
    bool isSubscriptionMessage = false;
    bool allowIgnore = true;
    bool isAction = false;
    QString channelPointRewardId = "";
};

struct HighlightAlert {
    QUrl customSound;
    bool playSound = false;
    bool windowAlert = false;
};
class MessageBuilder
{
public:
    /// Build a message without a base IRC message.
    MessageBuilder();

    MessageBuilder(SystemMessageTag, const QString &text,
                   const QTime &time = QTime::currentTime());
    MessageBuilder(TimeoutMessageTag, const QString &timeoutUser,
                   const QString &sourceUser, const QString &channel,
                   const QString &systemMessageText, uint32_t times,
                   const QDateTime &time);
    MessageBuilder(TimeoutMessageTag, const QString &username,
                   const QString &durationInSeconds, bool multipleTimes,
                   const QDateTime &time);
    MessageBuilder(const BanAction &action, const QDateTime &time,
                   uint32_t count = 1);
    MessageBuilder(const UnbanAction &action, const QDateTime &time);
    MessageBuilder(const WarnAction &action);
    MessageBuilder(const RaidAction &action);
    MessageBuilder(const UnraidAction &action);
    MessageBuilder(const AutomodUserAction &action);

    MessageBuilder(LiveUpdatesAddEmoteMessageTag, const QString &platform,
                   const QString &actor,
                   const std::vector<QString> &emoteNames);
    MessageBuilder(LiveUpdatesRemoveEmoteMessageTag, const QString &platform,
                   const QString &actor,
                   const std::vector<QString> &emoteNames);
    MessageBuilder(LiveUpdatesUpdateEmoteMessageTag, const QString &platform,
                   const QString &actor, const QString &emoteName,
                   const QString &oldEmoteName);
    MessageBuilder(LiveUpdatesUpdateEmoteSetMessageTag, const QString &platform,
                   const QString &actor, const QString &emoteSetName);

    /**
      * "Your image has been uploaded to %1[ (Deletion link: %2)]."
      * or "Your image has been uploaded to %1 %2. %3 left. "
      * "Please wait until all of them are uploaded. "
      * "About %4 seconds left."
      */
    MessageBuilder(ImageUploaderResultTag, const QString &imageLink,
                   const QString &deletionLink, size_t imagesStillQueued = 0,
                   size_t secondsLeft = 0);

    MessageBuilder(const MessageBuilder &) = delete;
    MessageBuilder(MessageBuilder &&) = delete;
    MessageBuilder &operator=(const MessageBuilder &) = delete;
    MessageBuilder &operator=(MessageBuilder &&) = delete;

    ~MessageBuilder() = default;

    Message *operator->();
    Message &message();
    MessagePtrMut release();
    std::weak_ptr<const Message> weakOf();

    void append(std::unique_ptr<MessageElement> element);
    void addLink(const linkparser::Parsed &parsedLink, const QString &source);

    template <typename T, typename... Args>
    T *emplace(Args &&...args)
    {
        static_assert(std::is_base_of<MessageElement, T>::value,
                      "T must extend MessageElement");

        auto unique = std::make_unique<T>(std::forward<Args>(args)...);
        auto pointer = unique.get();
        this->append(std::move(unique));
        return pointer;
    }

    void appendOrEmplaceText(const QString &text, MessageColor color);
    void appendOrEmplaceSystemTextAndUpdate(const QString &text,
                                            QString &toUpdate);

    // Helper method that emplaces some text stylized as system text
    // and then appends that text to the QString parameter "toUpdate".
    // Returns the TextElement that was emplaced.
    TextElement *emplaceSystemTextAndUpdate(const QString &text,
                                            QString &toUpdate);

    static void triggerHighlights(const Channel *channel,
                                  const HighlightAlert &alert);

    void appendChannelPointRewardMessage(const ChannelPointReward &reward,
                                         bool isMod, bool isBroadcaster);

    static MessagePtr makeChannelPointRewardMessage(
        const ChannelPointReward &reward, bool isMod, bool isBroadcaster);

    /// Make a "CHANNEL_NAME has gone live!" message
    static MessagePtr makeLiveMessage(const QString &channelName,
                                      const QString &channelID,
                                      MessageFlags extraFlags = {});

    // Messages in normal chat for channel stuff
    static MessagePtr makeOfflineSystemMessage(const QString &channelName,
                                               const QString &channelID);
    static MessagePtr makeHostingSystemMessage(const QString &channelName,
                                               bool hostOn);
    static MessagePtr makeDeletionMessageFromIRC(
        const MessagePtr &originalMessage);
    static MessagePtr makeDeletionMessageFromPubSub(const DeleteAction &action);
    static MessagePtr makeListOfUsersMessage(QString prefix, QStringList users,
                                             Channel *channel,
                                             MessageFlags extraFlags = {});
    static MessagePtr makeListOfUsersMessage(
        QString prefix, const std::vector<HelixModerator> &users,
        Channel *channel, MessageFlags extraFlags = {});

    static MessagePtr buildHypeChatMessage(Communi::IrcPrivateMessage *message);

    static std::pair<MessagePtr, MessagePtr> makeAutomodMessage(
        const AutomodAction &action, const QString &channelName);
    static MessagePtr makeAutomodInfoMessage(const AutomodInfoAction &action);

    static std::pair<MessagePtr, MessagePtr> makeLowTrustUserMessage(
        const PubSubLowTrustUsersMessage &action, const QString &channelName,
        const TwitchChannel *twitchChannel);
    static MessagePtr makeLowTrustUpdateMessage(
        const PubSubLowTrustUsersMessage &action);

    /// @brief Builds a message out of an `ircMessage`.
    ///
    /// Building a message won't cause highlights to be triggered. They will
    /// only be parsed. To trigger highlights (play sound etc.), use
    /// triggerHighlights().
    ///
    /// @param channel The channel this message was sent to. Must not be
    ///                `nullptr`.
    /// @param ircMessage The original message. This can be any message
    ///                   (PRIVMSG, USERNOTICE, etc.). Its content is not
    ///                   accessed through this parameter but through `content`,
    ///                   as the content might be inside a tag (e.g. gifts in a
    ///                   USERNOTICE).
    /// @param args Arguments from parsing a chat message.
    /// @param content The message text. This isn't always the entire text. In
    ///                replies, the leading mention can be cut off.
    ///                See `messageOffset`.
    /// @param messageOffset Starting offset to be used on index-based
    ///                      operations on `content` such as parsing emotes.
    ///                      For example:
    ///                         ircMessage = "@hi there"
    ///                         content = "there"
    ///                         messageOffset_ = 4
    ///                      The index 6 would resolve to 6 - 4 = 2 => 'e'
    /// @param thread The reply thread this message is part of. If there's no
    ///               thread, this is an empty `shared_ptr`.
    /// @param parent The direct parent this message is replying to. This does
    ///               not need to be the `thread`s root. If this message isn't
    ///               replying to anything, this is an empty `shared_ptr`.
    ///
    /// @returns The built message and a highlight result. If the message is
    ///          ignored (e.g. from a blocked user), then the returned pointer
    ///          will be en empty `shared_ptr`.
    static std::pair<MessagePtrMut, HighlightAlert> makeIrcMessage(
        Channel *channel, const Communi::IrcMessage *ircMessage,
        const MessageParseArgs &args, QString content,
        QString::size_type messageOffset,
        const std::shared_ptr<MessageThread> &thread = {},
        const MessagePtr &parent = {});

    static MessagePtrMut makeSystemMessageWithUser(
        const QString &text, const QString &loginName,
        const QString &displayName, const MessageColor &userColor,
        const QTime &time);

    static MessagePtrMut makeSubgiftMessage(const QString &text,
                                            const QVariantMap &tags,
                                            const QTime &time);

    /// "Chat has been cleared by a moderator." or "{actor} cleared the chat."
    /// @param actor The user who cleared the chat (empty if unknown)
    /// @param count How many times this message has been received already
    static MessagePtrMut makeClearChatMessage(const QDateTime &now,
                                              const QString &actor,
                                              uint32_t count = 1);

private:
    struct TextState {
        TwitchChannel *twitchChannel = nullptr;
        bool hasBits = false;
        bool bitsStacked = false;
        int bitsLeft = 0;
    };
    void addEmoji(const EmotePtr &emote);
    void addTextOrEmote(TextState &state, QString string);

    Outcome tryAppendCheermote(TextState &state, const QString &string);
    Outcome tryAppendEmote(TwitchChannel *twitchChannel, const EmoteName &name);

    bool isEmpty() const;
    MessageElement &back();
    std::unique_ptr<MessageElement> releaseBack();

    void parse();
    void parseUsernameColor(const QVariantMap &tags, const QString &userID);
    void parseUsername(const Communi::IrcMessage *ircMessage,
                       TwitchChannel *twitchChannel,
                       bool trimSubscriberUsername);
    void parseMessageID(const QVariantMap &tags);

    /// Parses the room-ID this message was received in
    ///
    /// @returns The room-ID
    static QString parseRoomID(const QVariantMap &tags,
                               TwitchChannel *twitchChannel);

    /// Parses the shared-chat information from this message.
    ///
    /// @param tags The tags of the received message
    /// @param twitchChannel The channel this message was received in
    /// @returns The source channel - the channel this message originated from.
    ///          If there's no channel currently open, @a twitchChannel is
    ///          returned.
    TwitchChannel *parseSharedChatInfo(const QVariantMap &tags,
                                       TwitchChannel *twitchChannel);

    // Parse & build thread information into the message
    // Will read information from thread_ or from IRC tags
    void parseThread(const QString &messageContent, const QVariantMap &tags,
                     const Channel *channel,
                     const std::shared_ptr<MessageThread> &thread,
                     const MessagePtr &parent);
    // parseHighlights only updates the visual state of the message, but leaves the playing of alerts and sounds to the triggerHighlights function
    HighlightAlert parseHighlights(const QVariantMap &tags,
                                   const QString &originalMessage,
                                   const MessageParseArgs &args);

    void appendChannelName(const Channel *channel);
    void appendUsername(const QVariantMap &tags, const MessageParseArgs &args);

    void addWords(const QStringList &words,
                  const std::vector<TwitchEmoteOccurrence> &twitchEmotes,
                  TextState &state);

    void appendTwitchBadges(const QVariantMap &tags,
                            TwitchChannel *twitchChannel);
    void appendChatterinoBadges(const QString &userID);
    void appendFfzBadges(TwitchChannel *twitchChannel, const QString &userID);
    void appendSeventvBadges(const QString &userID);

    [[nodiscard]] static bool isIgnored(const QString &originalMessage,
                                        const QString &userID,
                                        const Channel *channel);

    std::shared_ptr<Message> message_;
    MessageColor textColor_ = MessageColor::Text;

    QColor usernameColor_ = {153, 153, 153};
};

}  // namespace chatterino
