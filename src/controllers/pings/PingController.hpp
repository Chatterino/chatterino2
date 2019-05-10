#pragma once

#include <QObject>

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class Settings;
class Paths;

class PingModel;

class PingController final : public Singleton, private QObject
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    bool isMuted(const QString &channelName);
    void muteChannel(const QString &channelName);
    void unmuteChannel(const QString &channelName);
    bool toggleMuteChannel(const QString &channelName);

    PingModel *createModel(QObject *parent);

private:
    bool initialized_ = false;

    UnsortedSignalVector<QString> channelVector;

    ChatterinoSetting<std::vector<QString>> pingSetting_ = {"/pings/muted"};
};

}  // namespace chatterino
