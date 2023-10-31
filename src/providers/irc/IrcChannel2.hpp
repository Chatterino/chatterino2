#pragma once

#include "common/Channel.hpp"
#include "common/ChannelChatters.hpp"

namespace chatterino {

class Irc;
class IrcServer;

class IrcChannel final : public Channel, public ChannelChatters
{
public:
    explicit IrcChannel(const QString &name, IrcServer *server);
    ~IrcChannel();

    void sendMessage(const QString &message) override;

    // server may be nullptr
    IrcServer *server();

    // Channel methods
    bool canReconnect() const override;
    const QString &logFolderName() override;
    void reconnect() override;

private:
    void setServer(IrcServer *server);

    IrcServer *server_;
    QString logFolderName_;

    friend class Irc;
};

}  // namespace chatterino
