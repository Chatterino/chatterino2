#pragma once

#include "messages/MessageBuilder.hpp"
#include "messages/MessageParseArgs.hpp"
#include "singletons/EmoteManager.hpp"

#include <IrcMessage>

#include <QString>
#include <QVariant>

namespace chatterino {

class Channel;
class TwitchChannel;

class TwitchMessageBuilder : public chatterino::MessageBuilder
{
public:
    enum UsernameDisplayMode : int {
        Username = 1,                  // Username
        LocalizedName = 2,             // Localized name
        UsernameAndLocalizedName = 3,  // Username (Localized name)
    };

    TwitchMessageBuilder() = delete;

    explicit TwitchMessageBuilder(Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
                                  const chatterino::MessageParseArgs &_args);
    explicit TwitchMessageBuilder(Channel *_channel, const Communi::IrcMessage *_ircMessage,
                                  const chatterino::MessageParseArgs &_args, QString content,
                                  bool isAction);

    Channel *channel;
    TwitchChannel *twitchChannel;
    const Communi::IrcMessage *ircMessage;
    chatterino::MessageParseArgs args;
    const QVariantMap tags;

    QString messageID;
    QString userName;

    bool isIgnored() const;
    chatterino::MessagePtr build();

private:
    QString roomID;

    QColor usernameColor;
    const QString originalMessage;
    bool senderIsBroadcaster{};

    const bool action = false;

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
};

}  // namespace chatterino
