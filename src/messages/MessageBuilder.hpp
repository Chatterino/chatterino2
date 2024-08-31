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
#include <optional>
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

const SystemMessageTag systemMessage{};
const TimeoutMessageTag timeoutMessage{};
const LiveUpdatesUpdateEmoteMessageTag liveUpdatesUpdateEmoteMessage{};
const LiveUpdatesRemoveEmoteMessageTag liveUpdatesRemoveEmoteMessage{};
const LiveUpdatesAddEmoteMessageTag liveUpdatesAddEmoteMessage{};
const LiveUpdatesUpdateEmoteSetMessageTag liveUpdatesUpdateEmoteSetMessage{};

// This signifies that you want to construct a message containing the result of
// a successful image upload.
const ImageUploaderResultTag imageUploaderResultMessage{};

MessagePtr makeSystemMessage(const QString &text);
MessagePtr makeSystemMessage(const QString &text, const QTime &time);

struct MessageParseArgs {
    bool disablePingSounds = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool trimSubscriberUsername = false;
    bool isStaffOrBroadcaster = false;
    bool isSubscriptionMessage = false;
    QString channelPointRewardId = "";
};

struct TwitchEmoteOccurrence {
    int start;
    int end;
    EmotePtr ptr;
    EmoteName name;

    bool operator==(const TwitchEmoteOccurrence &other) const
    {
        return std::tie(this->start, this->end, this->ptr, this->name) ==
               std::tie(other.start, other.end, other.ptr, other.name);
    }
};

class MessageBuilder
{
public:
    /// Build a message without a base IRC message.
    MessageBuilder();

    /// Build a message based on an incoming IRC PRIVMSG
    explicit MessageBuilder(Channel *_channel,
                            const Communi::IrcPrivateMessage *_ircMessage,
                            const MessageParseArgs &_args);

    /// Build a message based on an incoming IRC message (e.g. notice)
    explicit MessageBuilder(Channel *_channel,
                            const Communi::IrcMessage *_ircMessage,
                            const MessageParseArgs &_args, QString content,
                            bool isAction);

    MessageBuilder(SystemMessageTag, const QString &text,
                   const QTime &time = QTime::currentTime());
    MessageBuilder(TimeoutMessageTag, const QString &timeoutUser,
                   const QString &sourceUser, const QString &systemMessageText,
                   int times, const QTime &time = QTime::currentTime());
    MessageBuilder(TimeoutMessageTag, const QString &username,
                   const QString &durationInSeconds, bool multipleTimes,
                   const QTime &time = QTime::currentTime());
    MessageBuilder(const BanAction &action, uint32_t count = 1);
    MessageBuilder(const UnbanAction &action);
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

    QString userName;

    TwitchChannel *twitchChannel = nullptr;

    Message *operator->();
    Message &message();
    MessagePtr release();
    std::weak_ptr<Message> weakOf();

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

    [[nodiscard]] bool isIgnored() const;
    bool isIgnoredReply() const;
    void triggerHighlights();
    MessagePtr build();

    void setThread(std::shared_ptr<MessageThread> thread);
    void setParent(MessagePtr parent);
    void setMessageOffset(int offset);

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

    static std::unordered_map<QString, QString> parseBadgeInfoTag(
        const QVariantMap &tags);

    // Parses "badges" tag which contains a comma separated list of key-value elements
    static std::vector<Badge> parseBadgeTag(const QVariantMap &tags);

    static std::vector<TwitchEmoteOccurrence> parseTwitchEmotes(
        const QVariantMap &tags, const QString &originalMessage,
        int messageOffset);

    static void processIgnorePhrases(
        const std::vector<IgnorePhrase> &phrases, QString &originalMessage,
        std::vector<TwitchEmoteOccurrence> &twitchEmotes);

protected:
    void addTextOrEmoji(EmotePtr emote);
    void addTextOrEmoji(const QString &string_);

    bool isEmpty() const;
    MessageElement &back();
    std::unique_ptr<MessageElement> releaseBack();

    MessageColor textColor_ = MessageColor::Text;

    // Helper method that emplaces some text stylized as system text
    // and then appends that text to the QString parameter "toUpdate".
    // Returns the TextElement that was emplaced.
    TextElement *emplaceSystemTextAndUpdate(const QString &text,
                                            QString &toUpdate);

    std::shared_ptr<Message> message_;

    void parse();
    void parseUsernameColor();
    void parseUsername();
    void parseMessageID();
    void parseRoomID();
    // Parse & build thread information into the message
    // Will read information from thread_ or from IRC tags
    void parseThread();
    // parseHighlights only updates the visual state of the message, but leaves the playing of alerts and sounds to the triggerHighlights function
    void parseHighlights();
    void appendChannelName();
    void appendUsername();

    Outcome tryAppendEmote(const EmoteName &name);

    void addWords(const QStringList &words,
                  const std::vector<TwitchEmoteOccurrence> &twitchEmotes);

    void appendTwitchBadges();
    void appendChatterinoBadges();
    void appendFfzBadges();
    void appendSeventvBadges();
    Outcome tryParseCheermote(const QString &string);

    bool shouldAddModerationElements() const;

    QString roomID_;
    bool hasBits_ = false;
    QString bits;
    int bitsLeft{};
    bool bitsStacked = false;
    bool historicalMessage_ = false;
    std::shared_ptr<MessageThread> thread_;
    MessagePtr parent_;

    /**
     * Starting offset to be used on index-based operations on `originalMessage_`.
     *
     * For example:
     * originalMessage_ = "there"
     * messageOffset_ = 4
     * (the irc message is "hey there")
     *
     * then the index 6 would resolve to 6 - 4 = 2 => 'e'
     */
    int messageOffset_ = 0;

    QString userId_;
    bool senderIsBroadcaster{};

    Channel *channel = nullptr;
    const Communi::IrcMessage *ircMessage;
    MessageParseArgs args;
    const QVariantMap tags;
    QString originalMessage_;

    const bool action_{};

    QColor usernameColor_ = {153, 153, 153};

    bool highlightAlert_ = false;
    bool highlightSound_ = false;
    std::optional<QUrl> highlightSoundCustomUrl_{};
};

}  // namespace chatterino
