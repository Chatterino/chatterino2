#ifndef CHANNEL_H
#define CHANNEL_H

#include "QString"
#include "QMap"

class Channel
{
public:
    static const Channel whispers;
    static const Channel mentions;

    static Channel* addChannel(const QString &channel);
    static Channel* getChannel(const QString &channel);
    static void  removeChannel(const QString &channel);

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

    static QMap<QString, Channel*> channels;

    int referenceCount = 1;

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
