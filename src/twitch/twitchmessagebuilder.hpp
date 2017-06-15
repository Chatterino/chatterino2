#pragma once

#include "channel.hpp"
#include "messages/messagebuilder.hpp"
#include "resources.hpp"

#include <QString>
#include <QVariant>

namespace chatterino {

class EmoteManager;
class WindowManager;

namespace twitch {

class TwitchMessageBuilder : public messages::MessageBuilder
{
public:
    TwitchMessageBuilder();

    QString messageId;
    QString userName;

    static messages::SharedMessage parse(const Communi::IrcPrivateMessage *ircMessage,
                                         Channel *channel, const messages::MessageParseArgs &args,
                                         Resources &resources, EmoteManager &emoteManager,
                                         WindowManager &windowManager);

    //    static bool sortTwitchEmotes(
    //        const std::pair<long int, messages::LazyLoadedImage *> &a,
    //        const std::pair<long int, messages::LazyLoadedImage *> &b);

private:
    void appendModerationWords(const Communi::IrcPrivateMessage *ircMessage,
                               const Resources &resources);
    void appendTwitchEmote(const Communi::IrcPrivateMessage *ircMessage, const QString &emote,
                           std::vector<std::pair<long int, messages::LazyLoadedImage *>> &vec,
                           EmoteManager &emoteManager);
    void appendTwitchBadges(const QStringList &badges, Resources &resources,
                            EmoteManager &emoteManager, const Resources::Channel &channelResources);
};

}  // namespace twitch
}  // namespace chatterino
