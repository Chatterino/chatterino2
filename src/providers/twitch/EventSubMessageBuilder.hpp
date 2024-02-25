#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/SharedMessageBuilder.hpp"
#include "providers/twitch/TwitchCommon.hpp"

#include <QString>
#include <QVariant>
#include <twitch-eventsub-ws/payloads/channel-chat-message-v1.hpp>

#include <optional>
#include <unordered_map>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Channel;
class TwitchChannel;
class MessageThread;
struct HelixVip;
using HelixModerator = HelixVip;
struct ChannelPointReward;
struct DeleteAction;

class EventSubMessageBuilder : MessageBuilder
{
    const eventsub::payload::channel_chat_message::v1::Payload &payload;
    const std::shared_ptr<Channel> channel;
    const MessageParseArgs &args;
    QString originalMessage;
    const TwitchChannel *twitchChannel;

    QColor usernameColor = {153, 153, 153};

public:
    EventSubMessageBuilder() = delete;

    EventSubMessageBuilder(const EventSubMessageBuilder &) = delete;
    EventSubMessageBuilder &operator=(const EventSubMessageBuilder &) = delete;

    EventSubMessageBuilder(EventSubMessageBuilder &&) = delete;
    EventSubMessageBuilder &operator=(EventSubMessageBuilder &&) = delete;
    /**
     * NOTE: The builder MUST NOT survive longer than the payload
     **/
    explicit EventSubMessageBuilder(
        const eventsub::payload::channel_chat_message::v1::Payload &_payload,
        const MessageParseArgs &_args);

    ~EventSubMessageBuilder() override = default;

    MessagePtr build();

    void setThread(std::shared_ptr<MessageThread> thread);
    void setParent(MessagePtr parent);
    void setMessageOffset(int offset);

private:
    void parseUsernameColor();
    // Parse & build thread information into the message
    // Will read information from thread_ or from IRC tags
    void parseThread();

    void runIgnoreReplaces(std::vector<TwitchEmoteOccurrence> &twitchEmotes);

    std::optional<EmotePtr> getTwitchBadge(const Badge &badge) const;
    Outcome tryAppendEmote(const EmoteName &name);

    void addWords();
    void addTextOrEmoji(EmotePtr emote) override;
    void addTextOrEmoji(const QString &value) override;

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

    // User ID of the sender of this message
    QString userId_;
    QString userName;  // TODO: Rename to userLogin
    bool senderIsBroadcaster{};

    const QVariantMap tags;
};

}  // namespace chatterino
