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

    QString messageID;
    QString userName;

    bool isIgnored() const;
    MessagePtr build();

private:
    void parseMessageID();
    void parseRoomID();
    void appendChannelName();
    void parseUsername();
    void appendUsername();
    void parseHighlights(bool isPastMsg);

    void appendTwitchEmote(const QString &emote,
                           std::vector<std::pair<int, EmotePtr>> &vec);
    Outcome tryAppendEmote(const EmoteName &name);

    void addWords(const QStringList &words,
                  const std::vector<std::pair<int, EmotePtr>> &twitchEmotes);
    void addTextOrEmoji(EmotePtr emote);
    void addTextOrEmoji(const QString &value);

    void appendTwitchBadges();
    void appendChatterinoBadges();
    Outcome tryParseCheermote(const QString &string);

    QString roomID_;
    bool hasBits_ = false;

    QColor usernameColor_;
    const QString originalMessage_;
    bool senderIsBroadcaster{};

    const bool action_ = false;
};

}  // namespace chatterino
