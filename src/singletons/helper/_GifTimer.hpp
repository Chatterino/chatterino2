#pragma once

#include <QTimer>
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace singletons {

class GIFTimer
{
public:
    void initialize();

    pajlada::Signals::NoArgSignal signal;

private:
    QTimer timer;
};

}  // namespace singletons
}  // namespace chatterino
