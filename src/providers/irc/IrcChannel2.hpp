#pragma once

#include "common/Channel.hpp"

namespace chatterino {

class Irc;
class IrcServer;

class IrcChannel : public Channel
{
public:
    explicit IrcChannel(const QString &name, IrcServer *server);

    void sendMessage(const QString &message) override;

    // server may be nullptr
    IrcServer *server();

private:
    void setServer(IrcServer *server);

    IrcServer *server_;

    friend class Irc;
};

}  // namespace chatterino
