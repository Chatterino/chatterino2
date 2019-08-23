#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/MessageBuilder.hpp"

#include <IrcMessage>
#include <QString>
#include <QVariant>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Channel;
class TwitchChannel;

class TwitchMessageBuilder : public MessageBuilder
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

    Channel *channel;
    TwitchChannel *twitchChannel;
    const Communi::IrcMessage *ircMessage;
    MessageParseArgs args;
    const QVariantMap tags;

    QString userName;

    [[nodiscard]] bool isIgnored() const;
    // triggerHighlights triggers any alerts or sounds parsed by parseHighlights
    void triggerHighlights();
    MessagePtr build();

private:
    void parseMessageID();
    void parseRoomID();
    void appendChannelName();
    void parseUsernameColor();
    void parseUsername();
    void appendUsername();
    void runIgnoreReplaces(
        std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes);
    // parseHighlights only updates the visual state of the message, but leaves the playing of alerts and sounds to the triggerHighlights function
    void parseHighlights();

    void appendTwitchEmote(
        const QString &emote,
        std::vector<std::tuple<int, EmotePtr, EmoteName>> &vec,
        std::vector<int> &correctPositions);
    Outcome tryAppendEmote(const EmoteName &name);

    void addWords(
        const QStringList &words,
        const std::vector<std::tuple<int, EmotePtr, EmoteName>> &twitchEmotes);
    void addTextOrEmoji(EmotePtr emote);
    void addTextOrEmoji(const QString &value);

    void appendTwitchBadges();
    void appendChatterinoBadges();
    Outcome tryParseCheermote(const QString &string);

    QString roomID_;
    bool hasBits_ = false;
    bool historicalMessage_ = false;

    QString userId_;
    QColor usernameColor_;
    QString originalMessage_;
    bool senderIsBroadcaster{};

    const bool action_ = false;

    bool highlightVisual_ = false;
    bool highlightAlert_ = false;
    bool highlightSound_ = false;
};

}  // namespace chatterino
