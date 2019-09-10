#pragma once

#include "providers/irc/AbstractIrcServer.hpp"
#include "providers/irc/IrcAccount.hpp"

namespace chatterino {

struct IrcConnection_;

class IrcServer : public AbstractIrcServer
{
public:
    explicit IrcServer(const IrcConnection_ &data);
    IrcServer(const IrcConnection_ &data,
              const std::vector<std::weak_ptr<Channel>> &restoreChannels);
    ~IrcServer() override;

    int id();
    const QString &user();
    const QString &nick();

    // AbstractIrcServer interface
protected:
    void initializeConnection(IrcConnection *connection, bool isRead,
                              bool isWrite) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;
    bool hasSeparateWriteConnection() const override;
    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void readConnectionMessageReceived(Communi::IrcMessage *message) override;

private:
    // pointer so we don't have to circle include Irc2.hpp
    IrcConnection_ *data_;
};

}  // namespace chatterino
