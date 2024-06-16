#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <vector>

namespace chatterino {

template <class ResultT>
class PronounsApiRequest
{
public:
    using ResultsT = std::vector<ResultT>;
    using CallbackT = std::function<void(ResultsT &&)>;

private:
    std::mutex mutex;
    std::size_t outstandingSubRequests;
    ResultsT results;
    CallbackT onDone;

public:
    // Creates a PronounsApiRequest awaiting numRequests calls to finishRequest.
    // If numRequests calls to finishRequest have been made, onDone is called and
    // this object is invalidated.
    // If more than numRequests calls to finishRequest are made, the behaviour is
    // undefined due to the class member results being moved out of this object.
    PronounsApiRequest(std::size_t numRequests, CallbackT &&onDone)
        : outstandingSubRequests{numRequests}
        , onDone{onDone}
    {
    }

    // Adds result to the list of results.
    // If this function call finishes the last outstanding sub request,
    // this object is invalidated, as the results are moved to onDone.
    void finishRequest(ResultT result)
    {
        bool done{false};

        {  // lock(mutex)
            std::lock_guard<std::mutex> lock(this->mutex);

            this->outstandingSubRequests--;
            this->results.push_back(result);

            if (this->outstandingSubRequests == 0)
            {
                // Finally done.
                done = true;
            }
        }  // lock(mutex)

        if (done)
        {
            this->onDone(std::move(this->results));
        }
    }
};

}  // namespace chatterino
