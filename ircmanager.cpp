#include "ircmanager.h"
#include "ircconnection.h"
#include "irccommand.h"
#include "future"
#include "QThreadPool"
#include "QRunnable"
#include "lambdaqrunnable.h"

IrcConnection* IrcManager::connection = NULL;
QMutex* IrcManager::connectionMutex = new QMutex();
long IrcManager::connectionIteration = 0;

IrcManager::IrcManager()
{

}

void IrcManager::connect()
{
    disconnect();

    QThreadPool::globalInstance()->start(new LambdaQRunnable([]{ beginConnecting(); return false; }));
}

void IrcManager::beginConnecting()
{
    int iteration = ++connectionIteration;

    auto c = new IrcConnection();

    QObject::connect(c,
                     &IrcConnection::messageReceived,
                     &messageReceived);
    QObject::connect(c,
                     &IrcConnection::privateMessageReceived,
                     &privateMessageReceived);

    c->setHost("irc.chat.twitch.tv");
    c->setPort(6667);

    c->setUserName("justinfan123");
    c->setNickName("justinfan123");
    c->setRealName("justinfan123");
    c->sendRaw("JOIN #hsdogdog");

    c->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    c->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    c->open();

    connectionMutex->lock();
    if (iteration == connectionIteration) {
        connection = c;
    }
    else {
        delete c;
    }
    connectionMutex->unlock();
}

void IrcManager::disconnect()
{
    connectionMutex->lock();

    if (connection != NULL) {
        delete connection;
        connection = NULL;
    }

    connectionMutex->unlock();
}

void IrcManager::messageReceived(IrcMessage *message)
{
//    qInfo(message->());
}

void IrcManager::privateMessageReceived(IrcPrivateMessage *message)
{
    qInfo(message->content().toStdString().c_str());
}
