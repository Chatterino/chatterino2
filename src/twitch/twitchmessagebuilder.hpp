#pragma once

#include "messages/messagebuilder.hpp"
#include "resources.hpp"

#include <QString>
#include <QVariant>

namespace chatterino {

class EmoteManager;
class WindowManager;
class Channel;
class ColorScheme;

namespace twitch {

class TwitchMessageBuilder : public messages::MessageBuilder
{
public:
    TwitchMessageBuilder() = delete;

    explicit TwitchMessageBuilder(Channel *_channel, Resources &_resources,
                                  EmoteManager &_emoteManager, WindowManager &_windowManager,
                                  const Communi::IrcPrivateMessage *_ircMessage,
                                  const messages::MessageParseArgs &_args);

    Channel *channel;
    Resources &resources;
    WindowManager &windowManager;
    ColorScheme &colorScheme;
    EmoteManager &emoteManager;
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
    std::string roomID;

    QColor usernameColor;

    void parseMessageID();
    void parseRoomID();
    void parseChannelName();
    void parseUsername();

    void appendModerationButtons();
    void appendTwitchEmote(const Communi::IrcPrivateMessage *ircMessage, const QString &emote,
                           std::vector<std::pair<long int, messages::LazyLoadedImage *>> &vec,
                           EmoteManager &emoteManager);

    void parseTwitchBadges();
};

}  // namespace twitch
}  // namespace chatterino
