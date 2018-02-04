#pragma once

#include "messages/messagebuilder.hpp"
#include "messages/messageparseargs.hpp"
#include "singletons/emotemanager.hpp"

#include <IrcMessage>

#include <QString>
#include <QVariant>

namespace chatterino {
class Channel;

namespace twitch {
class TwitchChannel;

class TwitchMessageBuilder : public messages::MessageBuilder
{
public:
    enum UsernameDisplayMode : int {
        Username = 1,                  // Username
        LocalizedName = 2,             // Localized name
        UsernameAndLocalizedName = 3,  // Username (Localized name)
    };

    TwitchMessageBuilder() = delete;

    explicit TwitchMessageBuilder(Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
                                  const messages::MessageParseArgs &_args);
    explicit TwitchMessageBuilder(Channel *_channel, const Communi::IrcMessage *_ircMessage,
                                  QString content, const messages::MessageParseArgs &_args);

    Channel *channel;
    TwitchChannel *twitchChannel;
    const Communi::IrcMessage *ircMessage;
    messages::MessageParseArgs args;
    const QVariantMap tags;

    QString messageID;
    QString userName;

    bool isIgnored() const;
    messages::MessagePtr build();

private:
    QString roomID;

    QColor usernameColor;
    const QString originalMessage;

    const bool action = false;

    void parseMessageID();
    void parseRoomID();
    void appendChannelName();
    void parseUsername();
    void appendUsername();
    void parseHighlights();

    void appendTwitchEmote(const Communi::IrcMessage *ircMessage, const QString &emote,
                           std::vector<std::pair<long, util::EmoteData>> &vec);
    bool tryAppendEmote(QString &emoteString);

    void appendTwitchBadges();
    void appendChatterinoBadges();
    bool tryParseCheermote(const QString &string);
};

}  // namespace twitch
}  // namespace chatterino
