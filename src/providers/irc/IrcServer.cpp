#include "IrcServer.hpp"

#include <cassert>

#include "messages/MessageBuilder.hpp"
#include "providers/irc/Irc2.hpp"
#include "providers/irc/IrcChannel2.hpp"

namespace chatterino {

IrcServer::IrcServer(const IrcServerData &data)
    : data_(new IrcServerData(data))
{
    this->connect();
}

IrcServer::IrcServer(const IrcServerData &data,
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

int IrcServer::id()
{
    return this->data_->id;
}

const QString &IrcServer::user()
{
    return this->data_->user;
}

const QString &IrcServer::nick()
{
    return this->data_->nick;
}

void IrcServer::initializeConnection(IrcConnection *connection, bool isRead,
                                     bool isWrite)
{
    assert(isRead && isWrite);

    connection->setSecure(this->data_->ssl);
    connection->setHost(this->data_->host);
    connection->setPort(this->data_->port);

    connection->setUserName(this->data_->user);
    connection->setNickName(this->data_->nick.isEmpty() ? this->data_->user
                                                        : this->data_->nick);
    connection->setRealName(this->data_->real.isEmpty() ? this->data_->user
                                                        : this->data_->nick);
    connection->setPassword(this->data_->password);
}

std::shared_ptr<Channel> IrcServer::createChannel(const QString &channelName)
{
    return std::make_shared<IrcChannel>(channelName, this);
}

bool IrcServer::hasSeparateWriteConnection() const
{
    return false;
}

void IrcServer::onReadConnected(IrcConnection *connection)
{
    AbstractIrcServer::onReadConnected(connection);

    std::lock_guard lock(this->channelMutex);

    for (auto &&weak : this->channels)
    {
        if (auto channel = weak.lock())
        {
            connection->sendRaw("JOIN #" + channel->getName());
        }
    }
}

void IrcServer::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
    auto target = message->target();
    target = target.startsWith('#') ? target.mid(1) : target;

    if (auto channel = this->getChannelOrEmpty(target); !channel->isEmpty())
    {
        MessageBuilder builder;

        builder.emplace<TimestampElement>();
        builder.emplace<TextElement>(message->nick() + ":",
                                     MessageElementFlag::Username);
        builder.emplace<TextElement>(message->content(),
                                     MessageElementFlag::Text);

        channel->addMessage(builder.release());
    }
}

void IrcServer::readConnectionMessageReceived(Communi::IrcMessage *message)
{
    qDebug() << QString(message->toData());
}

}  // namespace chatterino
