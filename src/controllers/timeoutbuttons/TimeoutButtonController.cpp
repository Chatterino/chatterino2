#include "TimeoutButtonController.hpp"

#include "controllers/timeoutbuttons/TimeoutButton.hpp"
#include "controllers/timeoutbuttons/TimeoutButtonModel.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchAccount.hpp"

namespace chatterino {

TimeoutButtonController::TimeoutButtonController()
{
}

TimeoutButtonModel *TimeoutButtonController::createModel(QObject *parent)
{
    TimeoutButtonModel *model = new TimeoutButtonModel(parent);

    // this breaks NaM - idk why
    model->init(&this->buttons);

    return model;
}

void TimeoutButtonController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    log("hallo");
    for (const TimeoutButton &button : this->timeoutButtons_.getValue())
    {
        this->buttons.appendItem(button);
    }
}
}  // namespace chatterino
