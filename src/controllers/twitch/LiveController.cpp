#include "controllers/twitch/LiveController.hpp"

#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/Helpers.hpp"

#include <QDebug>

namespace chatterino {

TwitchLiveController::TwitchLiveController()
{
    QObject::connect(&this->refreshTimer, &QTimer::timeout, [this] {
        this->request();
    });
    this->refreshTimer.start(60 * 1000);

    QObject::connect(&this->immediateRequestTimer, &QTimer::timeout, [this] {
        QStringList channelIDs;

        {
            std::unique_lock immediateRequestsLock(
                this->immediateRequestsMutex);
            for (const auto &channelID : this->immediateRequests)
            {
                channelIDs.append(channelID);
            }
            this->immediateRequests.clear();
        }

        this->request(channelIDs);
    });
    this->immediateRequestTimer.start(1 * 1000);

    qDebug() << "XD";
}

void TwitchLiveController::add(std::shared_ptr<TwitchChannel> newChannel)
{
    assert(newChannel != nullptr);

    const auto channelID = newChannel->roomId();
    assert(!channelID.isEmpty());

    qDebug() << "XXX: Add" << channelID;

    std::unique_lock lock(this->channelsMutex);
    auto &channelList = this->channels[channelID];

    if (channelList.empty())
    {
        std::unique_lock immediateRequestsLock(this->immediateRequestsMutex);
        this->immediateRequests.emplace(channelID);
    }

    channelList.emplace_back(newChannel);
}

void TwitchLiveController::remove(std::shared_ptr<TwitchChannel> channel)
{
    const auto channelID = channel->roomId();
    assert(!channelID.isEmpty());

    qDebug() << "XXX: Remove" << channelID;

    std::unique_lock lock(this->channelsMutex);
    auto &channelList = this->channels[channelID];

    channelList.erase(std::remove_if(channelList.begin(), channelList.end(),
                                     [](const auto &c) {
                                         return c.expired();
                                     }),
                      channelList.end());

    if (channelList.empty())
    {
        this->channels.erase(channelID);
    }
}

void TwitchLiveController::request(std::optional<QStringList> optChannelIDs)
{
    QStringList channelIDs;

    if (optChannelIDs)
    {
        qDebug() << "XXX Load requests from channels map" << channelIDs;
        channelIDs = *optChannelIDs;
    }
    else
    {
        std::shared_lock lock(this->channelsMutex);

        for (const auto &channelList : this->channels)
        {
            channelIDs.append(channelList.first);
        }
    }

    if (channelIDs.isEmpty())
    {
        qDebug() << "XXX: eraly out, no requests";
        return;
    }

    auto batches = splitListIntoBatches(channelIDs, 3);

    for (const auto &batch : batches)
    {
        qDebug() << "XXX MAKE REQUEST" << batch;

        // TODO: make concurrent
        getHelix()->fetchStreams(
            batch, {},
            [this, batch{batch}](const auto &streams) {
                // on success
                qDebug() << "XXX: success xd";
                std::unordered_map<QString, std::optional<HelixStream>> results;

                for (const auto &channelID : batch)
                {
                    results[channelID] = std::nullopt;
                }

                for (const auto &stream : streams)
                {
                    results[stream.userId] = stream;
                    qDebug() << "XXX: stream" << stream.userLogin;
                }

                {
                    std::unique_lock lock(this->channelsMutex);
                    QStringList deadChannels;
                    for (const auto &result : results)
                    {
                        auto it = this->channels.find(result.first);
                        if (it != channels.end())
                        {
                            const auto &weakChannelList = it->second;
                            for (const auto &weakChannel : weakChannelList)
                            {
                                auto channel = weakChannel.lock();
                                if (channel)
                                {
                                    qDebug() << "XXX: channel is alive"
                                             << channel->getName();
                                    channel->updateLiveStatus(result.second);
                                    // POST LIVE STATUS
                                }
                                else
                                {
                                    qDebug() << "XXX: channel is dead"
                                             << result.first;
                                    // channel is dead, mark as dead
                                    deadChannels.append(result.first);
                                }
                            }
                        }
                    }

                    for (const auto &deadChannel : deadChannels)
                    {
                        this->channels.erase(deadChannel);
                    }
                }
            },
            [] {
                qDebug() << "XXX: failed xd";
                // on failure
            },
            [] {
                // finally
            });
    }
}

}  // namespace chatterino
