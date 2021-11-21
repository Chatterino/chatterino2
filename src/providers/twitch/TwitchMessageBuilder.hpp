#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/SharedMessageBuilder.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/TwitchBadge.hpp"

#include <IrcMessage>
#include <QString>
#include <QVariant>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Channel;
class TwitchChannel;

struct TwitchEmoteOccurence {
    int start;
    int end;
    EmotePtr ptr;
    EmoteName name;
};

class TwitchMessageBuilder : public SharedMessageBuilder
{
public:
    TwitchMessageBuilder() = delete;

    explicit TwitchMessageBuilder(Channel *_channel,
                                  const Communi::IrcPrivateMessage *_ircMessage,
                                  const MessageParseArgs &_args);
    explicit TwitchMessageBuilder(Channel *_channel,
                                  const Communi::IrcMessage *_ircMessage,
                                  const MessageParseArgs &_args,
                                  QString content, bool isAction);

    TwitchChannel *twitchChannel;

    [[nodiscard]] bool isIgnored() const override;
    void triggerHighlights() override;
    MessagePtr build() override;

    static void appendChannelPointRewardMessage(
        const ChannelPointReward &reward, MessageBuilder *builder, bool isMod,
        bool isBroadcaster);

    // Message in the /live chat for channel going live
    static void liveMessage(const QString &channelName,
                            MessageBuilder *builder);

    // Messages in normal chat for channel stuff
    static void liveSystemMessage(const QString &channelName,
                                  MessageBuilder *builder);
    static void offlineSystemMessage(const QString &channelName,
                                     MessageBuilder *builder);
    static void hostingSystemMessage(const QString &channelName,
                                     MessageBuilder *builder, bool hostOn);
    static void deletionMessage(const MessagePtr originalMessage,
                                MessageBuilder *builder);
    static void deletionMessage(const DeleteAction &action,
                                MessageBuilder *builder);
    static void listOfUsersSystemMessage(QString prefix, QStringList users,
                                         Channel *channel,
                                         MessageBuilder *builder);

private:
    void parseUsernameColor() override;
    void parseUsername() override;
    void parseMessageID();
    void parseRoomID();
    void appendUsername();

    void runIgnoreReplaces(std::vector<TwitchEmoteOccurence> &twitchEmotes);

    boost::optional<EmotePtr> getTwitchBadge(const Badge &badge);
    void appendTwitchEmote(const QString &emote,
                           std::vector<TwitchEmoteOccurence> &vec,
                           std::vector<int> &correctPositions);
    Outcome tryAppendEmote(const EmoteName &name) override;

    void addWords(const QStringList &words,
                  const std::vector<TwitchEmoteOccurence> &twitchEmotes);
    void addTextOrEmoji(EmotePtr emote) override;
    void addTextOrEmoji(const QString &value) override;

    void appendTwitchBadges();
    void appendChatterinoBadges();
    void appendSeventvBadges();
    void appendFfzBadges();
    Outcome tryParseCheermote(const QString &string);

    bool shouldAddModerationElements() const;

    QString roomID_;
    bool hasBits_ = false;
    QString bits;
    int bitsLeft;
    bool bitsStacked = false;
    bool historicalMessage_ = false;

    QString userId_;
    bool senderIsBroadcaster{};
};

}  // namespace chatterino
