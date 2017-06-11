#pragma once

#include "channel.hpp"
#include "messages/messagebuilder.hpp"

#include <QString>

namespace chatterino {
namespace twitch {

class TwitchMessageBuilder : public messages::MessageBuilder
{
public:
    TwitchMessageBuilder();

    void appendTwitchBadges(const QStringList &badges);

    QString messageId;
    QString userName;

    static messages::SharedMessage parse(const Communi::IrcPrivateMessage *ircMessage,
                                         Channel *channel, const messages::MessageParseArgs &args);

    //    static bool sortTwitchEmotes(
    //        const std::pair<long int, messages::LazyLoadedImage *> &a,
    //        const std::pair<long int, messages::LazyLoadedImage *> &b);
};

}  // namespace twitch
}  // namespace chatterino
