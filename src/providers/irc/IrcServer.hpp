#pragma once

#include "providers/irc/AbstractIrcServer.hpp"

namespace chatterino {

struct IrcServerData;

class IrcServer : public AbstractIrcServer
{
public:
    explicit IrcServer(const IrcServerData &data);
    IrcServer(const IrcServerData &data,
              const std::vector<std::weak_ptr<Channel>> &restoreChannels);
    ~IrcServer() override;

    int id();
    const QString &user();
    const QString &nick();
    const QString &userFriendlyIdentifier();

    bool hasEcho() const;
    /**
     * @brief sends a whisper to the target user (PRIVMSG where a user is the target)
     */
    void sendWhisper(const QString &target, const QString &message);

    void sendRawMessage(const QString &rawMessage) override;

    // AbstractIrcServer interface
protected:
    void initializeConnectionSignals(IrcConnection *connection,
                                     ConnectionType type) override;
    void initializeConnection(IrcConnection *connection,
                              ConnectionType type) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;
    bool hasSeparateWriteConnection() const override;

    void onReadConnected(IrcConnection *connection) override;
    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void readConnectionMessageReceived(Communi::IrcMessage *message) override;

private:
    // pointer so we don't have to circle include Irc2.hpp
    IrcServerData *data_;

    bool hasEcho_{false};
};

}  // namespace chatterino
