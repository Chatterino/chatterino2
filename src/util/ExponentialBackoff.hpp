#pragma once

#include <chrono>

namespace chatterino {

// Yes, you can't specify the base 😎 deal with it
template <unsigned maxSteps>
class ExponentialBackoff
{
public:
    /**
     * Creates an object helping you make exponentially (with base 2) backed off times.
     *
     * @param start The start time in milliseconds
     * @param maxSteps The max number of progressions we will take before stopping
     *
     * For example, ExponentialBackoff(10ms, 3) would have the next() function return 10ms, 20ms, 40ms, 40ms, ..., 40ms
     **/
    ExponentialBackoff(const std::chrono::milliseconds &start)
        : start_(start)
    {
        static_assert(maxSteps > 1, "maxSteps must be higher than 1");
    }

    /**
     * Return the current number in the progression and increment the step until the next one (assuming we're not at the cap)
     *
     * @returns current step in milliseconds
     **/
    [[nodiscard]] std::chrono::milliseconds next()
    {
        auto next = this->start_ * (1 << (this->step_ - 1));

        this->step_ += 1;

        if (this->step_ >= maxSteps)
        {
            this->step_ = maxSteps;
        }

        return next;
    }

    /**
     * Reset the progression back to its initial state
     **/
    void reset()
    {
        this->step_ = 1;
    }

private:
    const std::chrono::milliseconds start_;
    unsigned step_ = 1;
};

}  // namespace chatterino
