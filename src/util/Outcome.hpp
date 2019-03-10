#pragma once

namespace chatterino
{
    struct SuccessTag
    {
    };

    struct FailureTag
    {
    };

    const SuccessTag Success{};
    const FailureTag Failure{};

    class Outcome
    {
    public:
        Outcome(SuccessTag)
            : success_(true)
        {
        }

        Outcome(FailureTag)
            : success_(false)
        {
        }

        explicit operator bool() const
        {
            return this->success_;
        }

        bool operator!() const
        {
            return !this->success_;
        }

        bool operator==(const Outcome& other) const
        {
            return this->success_ == other.success_;
        }

        bool operator!=(const Outcome& other) const
        {
            return !this->operator==(other);
        }

    private:
        bool success_;
    };
}  // namespace chatterino
