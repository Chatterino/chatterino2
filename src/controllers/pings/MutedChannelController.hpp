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

private:
    bool initialized_ = false;

    SignalVector<QString> channels;
};

}  // namespace chatterino
