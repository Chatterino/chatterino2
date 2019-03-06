#include "ui/ChannelView.Pauser.hpp"

namespace chatterino::ui
{
    Pauser::Pauser(QObject* parent)
        : QObject(parent)
    {
        this->pauseTimer_.setSingleShot(true);

        QObject::connect(
            &this->pauseTimer_, &QTimer::timeout, this, &Pauser::timeout);
    }

    bool Pauser::pausable() const
    {
        return pausable_;
    }

    void Pauser::setPausable(bool value)
    {
        this->pausable_ = value;
    }

    bool Pauser::paused() const
    {
        /// No elements in the map -> not paused
        return this->pausable() && !this->pauses_.empty();
    }

    void Pauser::pause(PauseReason reason, std::optional<uint> msecs)
    {
        if (msecs)
        {
            /// Msecs has a value
            auto timePoint =
                Clock::now() + std::chrono::milliseconds(msecs.value());
            auto it = this->pauses_.find(reason);

            if (it == this->pauses_.end())
            {
                /// No value found so we insert a new one.
                this->pauses_[reason] = timePoint;
            }
            else
            {
                /// If the new time point is newer then we override.
                if (it->second && it->second.value() < timePoint)
                    it->second = timePoint;
            }
        }
        else
        {
            /// Msecs is none -> pause is infinite.
            /// We just override the value.
            this->pauses_[reason] = std::nullopt;
        }

        this->updatePauseTimer();
    }

    void Pauser::unpause(PauseReason reason)
    {
        /// Remove the value from the map
        this->pauses_.erase(reason);

        this->updatePauseTimer();
    }

    // slot
    void Pauser::timeout()
    {
        /// remove elements that are finite
        for (auto it = this->pauses_.begin(); it != this->pauses_.end();)
            it = it->second ? this->pauses_.erase(it) : ++it;

        this->updatePauseTimer();
    }

    void Pauser::updatePauseTimer()
    {
        using namespace std::chrono;

        if (this->pauses_.empty())
        {
            /// No pauses so we can stop the timer
            this->pauseEnd_ = std::nullopt;
            this->pauseTimer_.stop();

            emit unpaused();
        }
        else if (std::any_of(this->pauses_.begin(), this->pauses_.end(),
                     [](auto&& value) { return !value.second; }))
        {
            /// Some of the pauses are infinite
            this->pauseEnd_ = std::nullopt;
            this->pauseTimer_.stop();
        }
        else
        {
            /// Get the maximum pause
            auto pauseEnd =
                std::max_element(this->pauses_.begin(), this->pauses_.end(),
                    [](auto&& a, auto&& b) { return a.second > b.second; })
                    ->second.value();

            if (pauseEnd != this->pauseEnd_)
            {
                /// Start the timer
                this->pauseEnd_ = pauseEnd;
                this->pauseTimer_.start(
                    duration_cast<milliseconds>(pauseEnd - Clock::now()));
            }
        }
    }
}  // namespace chatterino::ui
