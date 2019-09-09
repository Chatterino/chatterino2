#pragma once

#include <rapidjson/rapidjson.h>
#include <common/SignalVector.hpp>

#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"

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

    IrcServer *getServerOfChannel(Channel *channel);
    ChannelPtr getOrAddChannel(int serverId, QString name);

signals:
    void connectionUpdated(int id);

private:
    // Servers have a unique id.
    // When a server gets changed it gets removed and then added again.
    // So we store the channels of that server in abandonedChannels_ temporarily.
    // Or if the server got removed permanently then it's still stored there.
    std::unordered_map<int, std::unique_ptr<IrcServer>> servers_;
    std::unordered_map<int, std::vector<std::weak_ptr<Channel>>>
        abandonedChannels_;
};

}  // namespace chatterino
