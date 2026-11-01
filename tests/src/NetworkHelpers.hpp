// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "Test.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace chatterino {

class RequestWaiter
{
public:
    void requestDone()
    {
        {
            std::unique_lock lck(this->mutex_);
            ASSERT_FALSE(this->requestDone_);
            this->requestDone_ = true;
        }
        this->condition_.notify_one();
    }

    void waitForRequest(
        std::chrono::milliseconds interval = std::chrono::milliseconds(10))
    {
        using namespace std::chrono_literals;
        auto start = std::chrono::system_clock::now();

        while (true)
        {
            {
                std::unique_lock lck(this->mutex_);
                bool done = this->condition_.wait_for(lck, interval, [this] {
                    return this->requestDone_;
                });
                if (done)
                {
                    break;
                }
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

            if (std::chrono::system_clock::now() - start > 2min)
            {
                throw std::runtime_error("Timeout");
            }
        }

        ASSERT_TRUE(this->requestDone_);
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool requestDone_ = false;
};

}  // namespace chatterino
