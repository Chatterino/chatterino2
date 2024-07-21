#pragma once

#include "util/QStringHash.hpp"

#include <QString>
#include <QTimer>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace chatterino {

class TwitchChannel;

class ITwitchLiveController
{
public:
    virtual ~ITwitchLiveController() = default;

    virtual void add(const std::shared_ptr<TwitchChannel> &newChannel) = 0;
};

class TwitchLiveController : public ITwitchLiveController
{
public:
    // Controls how often all channels have their stream status refreshed
    static constexpr std::chrono::seconds REFRESH_INTERVAL{30};

    // Controls how quickly new channels have their stream status loaded
    static constexpr std::chrono::seconds IMMEDIATE_REQUEST_INTERVAL{1};

    /**
     * How many channels to include in a single request
     *
     * Should not be more than 100
     **/
    static constexpr int BATCH_SIZE{100};

    TwitchLiveController();

    // Add a Twitch channel to be queried for live status
    // A request is made within a few seconds if this is the first time this channel is added
    void add(const std::shared_ptr<TwitchChannel> &newChannel) override;

private:
    struct ChannelEntry {
        std::weak_ptr<TwitchChannel> ptr;
        bool wasChecked = false;
    };

    /**
     * Run batched Helix Channels & Stream requests for channels
     *
     * If a list of channel IDs is passed to request, we only make a request for those channels
     *
     * If no list of channels is passed to request (the default behaviour), we make requests for all channels
     * in the `channels` map.
     **/
    void request(std::optional<QStringList> optChannelIDs = std::nullopt);

    /**
     * List of channel IDs pointing to their Twitch Channel
     *
     * These channels will have their stream status updated every REFRESH_INTERVAL seconds
     **/
    std::unordered_map<QString, ChannelEntry> channels;
    std::shared_mutex channelsMutex;

    /**
     * List of channels that need an immediate live status update
     *
     * These channels will have their stream status updated after at most IMMEDIATE_REQUEST_INTERVAL seconds
     **/
    std::unordered_set<QString> immediateRequests;
    std::mutex immediateRequestsMutex;

    /**
     * Timer responsible for refreshing `channels`
     **/
    QTimer refreshTimer;

    /**
     * Timer responsible for refreshing `immediateRequests`
     **/
    QTimer immediateRequestTimer;
};

}  // namespace chatterino
