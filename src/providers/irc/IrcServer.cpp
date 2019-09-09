#include "IrcServer.hpp"

#include <cassert>

#include "providers/irc/Irc2.hpp"
#include "providers/irc/IrcChannel2.hpp"

namespace chatterino {

IrcServer::IrcServer(const IrcConnection_ &data)
    : data_(new IrcConnection_(data))
{
    this->connect();
}

IrcServer::IrcServer(const IrcConnection_ &data,
                     const std::vector<std::weak_ptr<Channel>> &restoreChannels)
    : IrcServer(data)
{
    for (auto &&weak : restoreChannels)
    {
        if (auto shared = weak.lock())
        {
            this->channels[shared->getName()] = weak;
        }
    }
}

IrcServer::~IrcServer()
{
    delete this->data_;
}

int IrcServer::getId()
{
    return this->data_->id;
}

void IrcServer::initializeConnection(IrcConnection *connection, bool isRead,
                                     bool isWrite)
{
    assert(isRead && isWrite);

    connection->setSecure(this->data_->ssl);
    connection->setHost(this->data_->host);
    connection->setPort(this->data_->port);

    connection->setUserName(this->data_->user);
    connection->setNickName(this->data_->nick);
    connection->setRealName(this->data_->nick);
    connection->setPassword(this->data_->password);
}

std::shared_ptr<Channel> IrcServer::createChannel(const QString &channelName)
{
    return std::make_shared<IrcChannel>(channelName);
}

bool IrcServer::hasSeparateWriteConnection() const
{
    return false;
}

}  // namespace chatterino
