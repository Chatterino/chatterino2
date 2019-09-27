#pragma once

#include <QObject>

#include "common/ChatterinoSetting.hpp"
#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"

namespace chatterino {

class TimeoutButton;

class TimeoutButtonModel;

class Settings;

class TimeoutButtonController final : public Singleton
{
public:
    TimeoutButtonController();

    TimeoutButtonModel *createModel(QObject *parent);

    virtual void initialize(Settings &settings, Paths &paths) override;

    UnsortedSignalVector<TimeoutButton> buttons;

private:
    bool initialized_ = false;

    ChatterinoSetting<std::vector<TimeoutButton>> timeoutButtons_ = {
        "/timeouts/timeoutButtons"};
};

}  // namespace chatterino
