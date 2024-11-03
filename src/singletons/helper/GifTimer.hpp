#pragma once

#include <pajlada/signals/signal.hpp>
#include <QTimer>

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

    void registerOpenOverlayWindow()
    {
        this->openOverlayWindows_++;
    }

    void unregisterOpenOverlayWindow()
    {
        assert(this->openOverlayWindows_ >= 1);
        this->openOverlayWindows_--;
    }

private:
    QTimer timer;
    long unsigned position_{};
    size_t openOverlayWindows_ = 0;
};

}  // namespace chatterino
