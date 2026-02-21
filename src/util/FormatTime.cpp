// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/FormatTime.hpp"

#include "common/QLogging.hpp"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <algorithm>
#include <limits>

namespace chatterino {

namespace {

using namespace Qt::Literals;

void appendShortDuration(int count, QChar suffix, QString &out)
{
    if (!out.isEmpty())
    {
        out.append(' ');
    }
    out.append(QString::number(count));
    out.append(suffix);
}

std::pair<uint16_t, boost::gregorian::date> yearsBetween(
    boost::gregorian::date first, boost::gregorian::date second)
{
    uint16_t years = 0;
    boost::gregorian::date lastOk = first;
    boost::gregorian::year_iterator yit(first);
    ++yit;  // skip one year
    for (; *yit <= second; ++yit)
    {
        years++;
        lastOk = *yit;
    }
    return {years, lastOk};
}

std::pair<uint16_t, boost::gregorian::date> monthsBetween(
    boost::gregorian::date first, boost::gregorian::date second)
{
    uint16_t months = 0;
    boost::gregorian::date lastOk = first;
    boost::gregorian::month_iterator mit(first);
    ++mit;  // skip one month
    for (; *mit <= second; ++mit)
    {
        months++;
        lastOk = *mit;
    }
    return {months, lastOk};
}

struct BalancedDuration {
    uint16_t years = 0;
    uint16_t months = 0;
    uint16_t days = 0;
    uint16_t hours = 0;
    uint16_t minutes = 0;
    uint16_t seconds = 0;

    using Component = std::pair<uint16_t BalancedDuration::*, QStringView>;
    constexpr static std::array<Component, 6> COMPONENTS{
        std::pair{&BalancedDuration::years, u"year"},
        {&BalancedDuration::months, u"month"},
        {&BalancedDuration::days, u"day"},
        {&BalancedDuration::hours, u"hour"},
        {&BalancedDuration::minutes, u"minute"},
        {&BalancedDuration::seconds, u"second"},
    };

    uint8_t components() const
    {
        uint8_t n = 0;
        for (const auto &[ptr, _name] : COMPONENTS)
        {
            if (this->*ptr != 0)
            {
                n++;
            }
        }
        return n;
    }
};

BalancedDuration durationBetween(const QDateTime &a, const QDateTime &b)
{
    auto fromDT = boost::posix_time::from_time_t(a.toSecsSinceEpoch());
    auto toDT = boost::posix_time::from_time_t(b.toSecsSinceEpoch());
    if (fromDT > toDT)
    {
        std::swap(fromDT, toDT);
    }

    auto fromD = fromDT.date();
    auto toD = toDT.date();

    if (toDT.time_of_day() < fromDT.time_of_day())
    {
        toD -= boost::gregorian::date_duration(1);
    }

    auto [years, yd] = yearsBetween(fromD, toD);
    auto [months, md] = monthsBetween(yd, toD);

    auto rem = toDT - boost::posix_time::ptime(md, fromDT.time_of_day());
    auto hoursAndDays = rem.hours();

    return {
        .years = years,
        .months = months,
        .days = static_cast<uint16_t>(hoursAndDays / 24),
        .hours = static_cast<uint16_t>(hoursAndDays % 24),
        .minutes = static_cast<uint16_t>(rem.minutes() % 60),
        .seconds = static_cast<uint16_t>(rem.seconds() % 60),
    };
}

}  // namespace

QString formatTime(int totalSeconds, int components)
{
    QString res;

    int seconds = totalSeconds % 60;
    int timeoutMinutes = totalSeconds / 60;
    int minutes = timeoutMinutes % 60;
    int timeoutHours = timeoutMinutes / 60;
    int hours = timeoutHours % 24;
    int days = timeoutHours / 24;
    if (days > 0 && components > 0)
    {
        appendShortDuration(days, 'd', res);
        components--;
    }
    if (hours > 0 && components > 0)
    {
        appendShortDuration(hours, 'h', res);
        components--;
    }
    if (minutes > 0 && components > 0)
    {
        appendShortDuration(minutes, 'm', res);
        components--;
    }
    if (seconds > 0 && components > 0)
    {
        appendShortDuration(seconds, 's', res);
        components--;
    }
    return res;
}

QString formatTime(const QString &totalSecondsString, int components)
{
    bool ok = true;
    int totalSeconds(totalSecondsString.toInt(&ok));
    if (ok)
    {
        return formatTime(totalSeconds, components);
    }

    return "n/a";
}

QString formatTime(std::chrono::seconds totalSeconds, int components)
{
    auto count = totalSeconds.count();

    return formatTime(
        static_cast<int>(std::clamp(count,
                                    static_cast<std::chrono::seconds::rep>(
                                        std::numeric_limits<int>::min()),
                                    static_cast<std::chrono::seconds::rep>(
                                        std::numeric_limits<int>::max()))),
        components);
}

QString formatLongFriendlyDuration(const QDateTime &from, const QDateTime &to)
{
    if (!from.isValid() || !to.isValid())
    {
        qCWarning(chatterinoHelper)
            << "Invalid arguments to formatLongFriendlyDuration - from:" << from
            << "to:" << to;
        return u"n/a"_s;
    }

    auto bd = durationBetween(from, to);
    auto remaining = std::min<uint8_t>(bd.components(), 4);
    const auto total = remaining;
    if (remaining == 0)
    {
        return u"0 seconds"_s;
    }

    QString out;
    for (const auto &[ptr, name] : BalancedDuration::COMPONENTS)
    {
        uint16_t n = bd.*ptr;
        if (n == 0)
        {
            continue;
        }

        if (!out.isEmpty())
        {
            if (remaining == 1)
            {
                if (total > 2)
                {
                    out += ',';
                }
                out += u" and ";
            }
            else
            {
                out += u", ";
            }
        }

        out += QString::number(n);
        out += u' ';
        out += name;
        if (n != 1)
        {
            out += u's';
        }

        --remaining;
        if (remaining == 0)
        {
            break;
        }
    }

    return out;
}

}  // namespace chatterino
