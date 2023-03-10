#pragma once

#include "common/Channel.hpp"
#include "common/ChannelChatters.hpp"

namespace chatterino {

class Irc;
class IrcServer;

class IrcChannel : public Channel, public ChannelChatters
{
public:
    explicit IrcChannel(const QString &name, IrcServer *server);

    void sendMessage(const QString &message) override;

    // server may be nullptr
    IrcServer *server();

    // Channel methods
    virtual bool canReconnect() const override;
    virtual void reconnect() override;

private:
    void setServer(IrcServer *server);

    IrcServer *server_;

    friend class Irc;
};

}  // namespace chatterino
