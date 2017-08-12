#pragma once

#include "emotemanager.hpp"
#include "messages/messagebuilder.hpp"
#include "resources.hpp"

#include <QString>
#include <QVariant>

namespace chatterino {

class WindowManager;
class Channel;
class ColorScheme;

namespace twitch {

class TwitchMessageBuilder : public messages::MessageBuilder
{
public:
    enum UsernameDisplayMode : int {
        Username = 1,                  // Username
        LocalizedName = 2,             // Localized name
        UsernameAndLocalizedName = 3,  // Username (Localized name)
    };

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
    void appendUsername();
    void parseHighlights();

    void appendModerationButtons();
    void appendTwitchEmote(const Communi::IrcPrivateMessage *ircMessage, const QString &emote,
                           std::vector<std::pair<long, EmoteData>> &vec,
                           EmoteManager &emoteManager);
    bool tryAppendEmote(QString &emoteString);
    bool appendEmote(EmoteData &emoteData);

    void parseTwitchBadges();
    void parseChatterinoBadges();
};

}  // namespace twitch
}  // namespace chatterino
