#ifndef TWITCHMESSAGEBUILDER_H
#define TWITCHMESSAGEBUILDER_H

#include "channel.h"
#include "messages/messagebuilder.h"

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
}
}
#endif  // TWITCHMESSAGEBUILDER_H
