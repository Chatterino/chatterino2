#ifndef CHANNEL_H
#define CHANNEL_H

#include "QString"
#include "QMap"
#include "QMutex"
#include "QVector"

class Message;

class Channel
{
public:
    static const Channel whispers;
    static const Channel mentions;

    static Channel* addChannel(const QString &channel);
    static Channel* getChannel(const QString &channel);
    static void  removeChannel(const QString &channel);

    QString getSubLink();
    QString getChannelLink();
    QString getPopoutPlayerLink();

    bool getIsLive();
    int getStreamViewerCount();
    QString getStreamStatus();
    QString getStreamGame();

    const QString& name() {
        return m_name;
    }

    void addMessage(Message* message);
//    ~Channel();

    QVector<Message*>* getMessagesClone();

private:
    Channel(QString channel);

    QMutex* messageMutex;

    static QMap<QString, Channel*> channels;

    int referenceCount = 1;

    QString m_name;

    int roomID;

    QVector<Message*>* messages = new QVector<Message*>();

    QString subLink = "";
    QString channelLink = "";
    QString popoutPlayerLink = "";

    bool isLive = false;
    int streamViewerCount = 0;
    QString streamStatus = "";
    QString streamGame = "";
};

#endif // CHANNEL_H
