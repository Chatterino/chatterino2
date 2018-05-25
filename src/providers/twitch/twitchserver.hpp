#pragma once

#include "providers/irc/abstractircserver.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "util/mutexvalue.h"

#include <memory>

namespace chatterino {
namespace providers {
namespace twitch {

class TwitchServer final : public irc::AbstractIrcServer
{
public:
    TwitchServer();

    void initialize();

    // fourtf: ugh
    void forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func);

    std::shared_ptr<Channel> getChannelOrEmptyByID(const QString &channelID);

    util::MutexValue<QString> lastUserThatWhisperedMe;

    //    QString getLastWhisperedPerson() const;
    //    void setLastWhisperedPerson(const QString &person);

    const ChannelPtr whispersChannel;
    const ChannelPtr mentionsChannel;
    IndirectChannel watchingChannel;

protected:
    void initializeConnection(Communi::IrcConnection *connection, bool isRead,
                              bool isWrite) override;
    std::shared_ptr<Channel> createChannel(const QString &channelName) override;

    void privateMessageReceived(Communi::IrcPrivateMessage *message) override;
    void messageReceived(Communi::IrcMessage *message) override;
    void writeConnectionMessageReceived(Communi::IrcMessage *message) override;

    std::shared_ptr<Channel> getCustomChannel(const QString &channelname) override;

    QString cleanChannelName(const QString &dirtyChannelName) override;

private:
    //    mutable std::mutex lastWhisperedPersonMutex;
    //    QString lastWhisperedPerson;
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
