#include "channel.h"

const Channel Channel::whispers = Channel(QString("/whispers"));
const Channel Channel::mentions = Channel(QString("/mentions"));

Channel::Channel(QString channel)
{
    name = (channel.length() > 0 && channel[0] == '#') ? channel.mid(1) : channel;
    subLink = "https://www.twitch.tv/" + name + "/subscribe?ref=in_chat_subscriber_link";
    channelLink = "https://twitch.tv/" + name;
    popoutPlayerLink = "https://player.twitch.tv/?channel=" + name;
}

QString Channel::getSubLink()            { return subLink          ; }
QString Channel::getChannelLink()        { return channelLink      ; }
QString Channel::getPopoutPlayerLink()   { return popoutPlayerLink ; }

bool    Channel::getIsLive()             { return isLive           ; }
int     Channel::getStreamViewerCount()  { return streamViewerCount; }
QString Channel::getStreamStatus()       { return streamStatus     ; }
QString Channel::getStreamGame()         { return streamGame       ; }
