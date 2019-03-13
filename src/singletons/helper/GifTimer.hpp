#pragma once

#include <QTimer>
#include <pajlada/signals/signal.hpp>

namespace chatterino
{
    class GIFTimer
    {
    public:
        void initialize();

        pajlada::Signals::NoArgSignal signal;

    private:
        QTimer timer;
    };
}  // namespace chatterino
