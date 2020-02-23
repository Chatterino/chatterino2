#pragma once

#include <QObject>

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class Settings;
class Paths;

class MutedChannelModel;

class MutedChannelController final : public Singleton, private QObject
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    bool isMuted(const QString &channelName);
    bool toggleMuted(const QString &channelName);

private:
    void mute(const QString &channelName);
    void unmute(const QString &channelName);
    bool initialized_ = false;

    SignalVector<QString> channels;
};

}  // namespace chatterino
