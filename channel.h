#ifndef CHANNEL_H
#define CHANNEL_H

#include "QString"

class Channel
{
public:
    static const Channel whispers;
    static const Channel mentions;

public:
    QString getSubLink();
    QString getChannelLink();
    QString getPopoutPlayerLink();

    bool getIsLive();
    int getStreamViewerCount();
    QString getStreamStatus();
    QString getStreamGame();

private:
    Channel(QString channel);

    int referenceCount = 0;

    QString name;

    int roomID;

    QString subLink = "";
    QString channelLink = "";
    QString popoutPlayerLink = "";

    bool isLive = false;
    int streamViewerCount = 0;
    QString streamStatus = "";
    QString streamGame = "";
};

#endif // CHANNEL_H
