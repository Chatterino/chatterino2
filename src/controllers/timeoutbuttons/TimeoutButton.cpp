#include "TimeoutButton.hpp"

namespace chatterino {

TimeoutButton::TimeoutButton(const int &duration, const QString &unit)
    : duration_(duration)
    , unit_(unit)
{
}

int TimeoutButton::getDuration() const
{
    return this->duration_;
}

QString TimeoutButton::getDurationString() const
{
    return QString::number(this->getDuration());
}

QString TimeoutButton::getUnit() const
{
    return this->unit_;
}

int TimeoutButton::getTimeoutDuration() const
{
    static const QMap<QString, int> durations{
        {"s", 1}, {"m", 60}, {"h", 3600}, {"d", 86400}, {"w", 604800},
    };
    return this->duration_ * durations[this->unit_];
}

}  // namespace chatterino
