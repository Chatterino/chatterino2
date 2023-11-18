#pragma once

#include <memory>
#include <thread>

namespace chatterino {

class EventSub
{
public:
    void start();

private:
    std::unique_ptr<std::thread> mainThread;
};

}  // namespace chatterino
