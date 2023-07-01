#pragma once

#include "common/Singleton.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/serialize/Container.hpp"

#include <boost/optional.hpp>
#include <QColor>
#include <QString>
#include <QTimer>

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

    virtual void add(std::shared_ptr<TwitchChannel> newChannel) = 0;
    virtual void remove(std::shared_ptr<TwitchChannel> channel) = 0;
};

class TwitchLiveController : public ITwitchLiveController, public Singleton
{
public:
    TwitchLiveController();

    void add(std::shared_ptr<TwitchChannel> newChannel) override;
    void remove(std::shared_ptr<TwitchChannel> channel) override;

private:
    void request(std::optional<QStringList> optChannelIDs = std::nullopt);

    // List of channel IDs pointing to Twitch Channels
    std::unordered_map<QString, std::vector<std::weak_ptr<TwitchChannel>>>
        channels;
    std::shared_mutex channelsMutex;

    QTimer refreshTimer;

    // List of channels that need an immediate live status update
    std::unordered_set<QString> immediateRequests;
    std::mutex immediateRequestsMutex;

    QTimer immediateRequestTimer;
};

}  // namespace chatterino
