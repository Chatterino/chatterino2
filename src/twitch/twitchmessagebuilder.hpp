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

    Channel *channel;
    TwitchChannel *twitchChannel;
    const Communi::IrcPrivateMessage *ircMessage;
    messages::MessageParseArgs args;
    const QVariantMap tags;

    QString messageID;
    QString userName;

    messages::SharedMessage parse();

    //    static bool sortTwitchEmotes(
    //        const std::pair<long int, messages::LazyLoadedImage *> &a,
    //        const std::pair<long int, messages::LazyLoadedImage *> &b);

private:
    QString roomID;

    QColor usernameColor;

    void parseMessageID();
    void parseRoomID();
    void parseChannelName();
    void parseUsername();
    void appendUsername();
    void parseHighlights();

    void appendModerationButtons();
    void appendTwitchEmote(const Communi::IrcPrivateMessage *ircMessage, const QString &emote,
                           std::vector<std::pair<long, util::EmoteData>> &vec);
    bool tryAppendEmote(QString &emoteString);
    bool appendEmote(util::EmoteData &emoteData);

    void parseTwitchBadges();
    void parseChatterinoBadges();
};

}  // namespace twitch
}  // namespace chatterino
