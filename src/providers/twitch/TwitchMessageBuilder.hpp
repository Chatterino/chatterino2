#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/SharedMessageBuilder.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
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
    enum UsernameDisplayMode : int {
        Username = 1,                  // Username
        LocalizedName = 2,             // Localized name
        UsernameAndLocalizedName = 3,  // Username (Localized name)
    };

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
        const ChannelPointReward &reward, MessageBuilder *builder);

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
    void appendFfzBadges();
    Outcome tryParseCheermote(const QString &string);

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
