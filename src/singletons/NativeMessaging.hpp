#pragma once

#include "common/Atomic.hpp"

#include <QString>
#include <QThread>

#include <optional>
#include <vector>

namespace chatterino {

class Application;
class Paths;
class Channel;

using ChannelPtr = std::shared_ptr<Channel>;

void registerNmHost(const Paths &paths);
std::string &getNmQueueName(const Paths &paths);

Atomic<std::optional<QString>> &nmIpcError();

namespace nm::client {

    void sendMessage(const QByteArray &array);
    void writeToCout(const QByteArray &array);

}  // namespace nm::client

class NativeMessagingServer final
{
public:
    NativeMessagingServer();
    NativeMessagingServer(const NativeMessagingServer &) = delete;
    NativeMessagingServer(NativeMessagingServer &&) = delete;
    NativeMessagingServer &operator=(const NativeMessagingServer &) = delete;
    NativeMessagingServer &operator=(NativeMessagingServer &&) = delete;
    ~NativeMessagingServer();

    void start();

private:
    class ReceiverThread : public QThread
    {
    public:
        ReceiverThread(NativeMessagingServer &parent);

        void run() override;

    private:
        void handleMessage(const QJsonObject &root);
        void handleSelect(const QJsonObject &root);
        void handleDetach(const QJsonObject &root);
        void handleSync(const QJsonObject &root);

        NativeMessagingServer &parent_;
    };

    void syncChannels(const QJsonArray &twitchChannels);

    ReceiverThread *thread;

    /// This vector contains all channels that are open the user's browser.
    /// These channels are joined to be able to switch channels more quickly.
    std::vector<ChannelPtr> channelWarmer_;

    friend ReceiverThread;
};

}  // namespace chatterino
