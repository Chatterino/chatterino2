#ifndef IRCMANAGER_H
#define IRCMANAGER_H

#define TWITCH_MAX_MESSAGELENGTH 500

#include "IrcMessage"
#include "QMutex"

class IrcManager
{
public:
    static void connect();
    static void disconnect();

private:
    IrcManager();

    static void beginConnecting();

    static QObject* parent;

    static IrcConnection* connection;
    static QMutex* connectionMutex;
    static long connectionIteration;

    static void messageReceived(IrcMessage* message);
    static void privateMessageReceived(IrcPrivateMessage* message);
};

#endif // IRCMANAGER_H
