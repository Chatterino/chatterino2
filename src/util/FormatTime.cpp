#include "util/FormatTime.hpp"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <algorithm>
#include <limits>

namespace chatterino {

namespace {

using namespace Qt::Literals;

void appendShortDuration(int count, QChar &&suffix, QString &out)
{
    if (!out.isEmpty())
    {
        out.append(' ');
    }
    out.append(QString::number(count));
    out.append(suffix);
}

std::pair<uint32_t, boost::gregorian::date> yearsBetween(
    boost::gregorian::date first, boost::gregorian::date second)
{
    uint32_t years = 0;
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

std::pair<uint32_t, boost::gregorian::date> monthsBetween(
    boost::gregorian::date first, boost::gregorian::date second)
{
    uint32_t months = 0;
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
    uint32_t years = 0;
    uint32_t months = 0;
    uint32_t days = 0;
    uint32_t hours = 0;

    uint8_t components() const
    {
        uint8_t c = 0;
        if (this->years > 0)
        {
            c++;
        }
        if (this->months > 0)
        {
            c++;
        }
        if (this->days > 0)
        {
            c++;
        }
        if (this->hours > 0)
        {
            c++;
        }
        return c;
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

    auto [years, yd] = yearsBetween(fromD, toD);
    auto [months, md] = monthsBetween(yd, toD);

    auto rem = toDT - boost::posix_time::ptime(md, fromDT.time_of_day());
    auto hoursAndDays = rem.hours();

    return {
        .years = years,
        .months = months,
        .days = static_cast<uint32_t>(hoursAndDays / 24),
        .hours = static_cast<uint32_t>(hoursAndDays % 24),
    };
}

}  // namespace

QString formatTime(int totalSeconds)
{
    QString res;

    int seconds = totalSeconds % 60;
    int timeoutMinutes = totalSeconds / 60;
    int minutes = timeoutMinutes % 60;
    int timeoutHours = timeoutMinutes / 60;
    int hours = timeoutHours % 24;
    int days = timeoutHours / 24;
    if (days > 0)
    {
        appendShortDuration(days, 'd', res);
    }
    if (hours > 0)
    {
        appendShortDuration(hours, 'h', res);
    }
    if (minutes > 0)
    {
        appendShortDuration(minutes, 'm', res);
    }
    if (seconds > 0)
    {
        appendShortDuration(seconds, 's', res);
    }
    return res;
}

QString formatTime(const QString &totalSecondsString)
{
    bool ok = true;
    int totalSeconds(totalSecondsString.toInt(&ok));
    if (ok)
    {
        return formatTime(totalSeconds);
    }

    return "n/a";
}

QString formatTime(std::chrono::seconds totalSeconds)
{
    auto count = totalSeconds.count();

    return formatTime(static_cast<int>(std::clamp(
        count,
        static_cast<std::chrono::seconds::rep>(std::numeric_limits<int>::min()),
        static_cast<std::chrono::seconds::rep>(
            std::numeric_limits<int>::max()))));
}

QString formatLongFriendlyDuration(const QDateTime &from, const QDateTime &to)
{
    auto bd = durationBetween(from, to);
    uint8_t remaining = bd.components();
    if (remaining == 0)
    {
        return u"0 hours"_s;
    }

    QString out;
    auto push = [&](uint32_t n, QStringView s) {
        if (n == 0)
        {
            return;
        }

        if (!out.isEmpty())
        {
            if (remaining == 1)
            {
                out += u" and ";
            }
            else
            {
                out += u", ";
            }
        }
        --remaining;

        out += QString::number(n);
        out += u' ';
        out += s;
        if (n != 1)
        {
            out += u's';
        }
    };
    push(bd.years, u"year");
    push(bd.months, u"month");
    push(bd.days, u"day");
    push(bd.hours, u"hour");

    return out;
}

}  // namespace chatterino
