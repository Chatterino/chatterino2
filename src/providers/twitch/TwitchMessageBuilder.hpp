#pragma once

#include "messages/MessageBuilder.hpp"
#include "messages/MessageParseArgs.hpp"
#include "singletons/Emotes.hpp"

#include <IrcMessage>

#include <QString>
#include <QVariant>

namespace chatterino {

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

    explicit TwitchMessageBuilder(Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
                                  const MessageParseArgs &_args);
    explicit TwitchMessageBuilder(Channel *_channel, const Communi::IrcMessage *_ircMessage,
                                  const MessageParseArgs &_args, QString content, bool isAction);

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
    void parseHighlights();

    void appendTwitchEmote(const Communi::IrcMessage *ircMessage, const QString &emote,
                           std::vector<std::pair<long, EmoteData>> &vec);
    bool tryAppendEmote(QString &emoteString);

    void appendTwitchBadges();
    void appendChatterinoBadges();
    bool tryParseCheermote(const QString &string);

    QString roomID_;

    QColor usernameColor_;
    QString originalMessage_;
    bool senderIsBroadcaster{};

    const bool action_ = false;
};

}  // namespace chatterino
