#pragma once

#include <memory>

namespace pajlada::Signals
{
    class SignalHolder;
}  // namespace pajlada::Signals

namespace chatterino
{
    class Button;
    class UpdateDialog;

    void initUpdateButton(
        Button& button, pajlada::Signals::SignalHolder& signalHolder);
}  // namespace chatterino
