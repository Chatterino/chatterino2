#pragma once

#include <memory>

namespace pajlada::Signals
{
    class SignalHolder;
}

namespace ab
{
    class FlatButton;
}

namespace chatterino
{
    class UpdateDialog;

    void initUpdateButton(
        ab::FlatButton& button, pajlada::Signals::SignalHolder& signalHolder);
}  // namespace chatterino
