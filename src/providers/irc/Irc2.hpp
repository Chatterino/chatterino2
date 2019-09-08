#pragma once

#include <common/SignalVector.hpp>

class QAbstractTableModel;

namespace chatterino {

struct IrcConnection_ {
    QString host;
    int port;
    bool ssl;

    QString user;
    QString nick;
    QString password;

    int id;

    // makes an IrcConnection with a unique id
    static IrcConnection_ unique();
};

class Irc
{
public:
    Irc();

    static Irc &getInstance();

    UnsortedSignalVector<IrcConnection_> connections;
    QAbstractTableModel *newConnectionModel(QObject *parent);

signals:
    void connectionUpdated(int id);
};

}  // namespace chatterino
