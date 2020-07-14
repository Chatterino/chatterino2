﻿#pragma once

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

private:
    void parseUsernameColor() override;
    void parseUsername() override;
    void parseMessageID();
    void parseRoomID();
    void appendUsername();
    void runIgnoreReplaces(
        std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes);

    boost::optional<EmotePtr> getTwitchBadge(const Badge &badge);
    void appendTwitchEmote(
        const QString &emote,
        std::vector<std::tuple<int, EmotePtr, EmoteName>> &vec,
        std::vector<int> &correctPositions);
    Outcome tryAppendEmote(const EmoteName &name);

    void addWords(
        const QStringList &words,
        const std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes);
    void addTextOrEmoji(EmotePtr emote) override;
    void addTextOrEmoji(const QString &value) override;

    void appendTwitchBadges();
    void appendChatterinoBadges();
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
