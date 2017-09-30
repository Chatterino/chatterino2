#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <QEventLoop>

namespace chatterino {
namespace messages {

class LazyLoadedImage;

class ImageLoader
{
public:
    ImageLoader();
    ~ImageLoader();
    void push_back(chatterino::messages::LazyLoadedImage *lli);

private:
    void run();
    std::thread worker;
    std::mutex workerM;
    std::vector<chatterino::messages::LazyLoadedImage *> toProcess;
    bool ready = false;
    std::condition_variable cv;
    bool exit = false;
    std::unique_ptr<QEventLoop> eventLoop;
};

}  // namespace messages
}  // namespace chatterino
