#pragma once

#include <QTimer>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

constexpr long unsigned GIF_FRAME_LENGTH = 20;

class GIFTimer
{
public:
    void initialize();

    pajlada::Signals::NoArgSignal signal;
    long unsigned position()
    {
        return this->position_;
    }

private:
    QTimer timer;
    long unsigned position_{};
};

}  // namespace chatterino
