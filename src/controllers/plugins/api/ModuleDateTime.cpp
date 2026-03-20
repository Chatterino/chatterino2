#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/ModuleDateTime.hpp"

#    include "controllers/plugins/LuaUtilities.hpp"

#    include <QDateTime>
#    include <QLocale>
#    include <QObject>
#    include <QStringView>
#    include <Qt>
#    include <QTimeZone>
#    include <sol/overload.hpp>
#    include <sol/raii.hpp>
#    include <sol/types.hpp>
#    include <sol/variadic_args.hpp>

#    include <stdexcept>

namespace chatterino::lua::api::qt {

static QString qdtMetaToString(QDateTime *self)
{
    const static QString format = "<qt/datetime.QDateTime: %1>";
    if (self->isNull())
    {
        return format.arg("Null");
    }
    return format.arg(self->toString(Qt::ISODate));
}

static QString qtMetaToString(QTime *self)
{
    const static QString format = "<qt/datetime.QTime: %1>";
    if (self->isNull())
    {
        return format.arg("Null");
    }
    return format.arg(self->toString(Qt::ISODate));
}

static QString qdMetaToString(QDate *self)
{
    const static QString format = "<qt/datetime.QDate: %1>";
    if (self->isNull())
    {
        return format.arg("Null");
    }
    return format.arg(self->toString(Qt::ISODate));
}

static QString qtzMetaToString(QTimeZone *self)
{
    const static QString format = "<qt/datetime.QTimeZone: %1>";
    if (!self->isValid())
    {
        return format.arg("Invalid");
    }
    return format.arg(self->abbreviation(QDateTime::currentDateTime()));
}

bool qdtEqual(const QDateTime &self, const QDateTime &other)
{
    return self == other;
}

sol::object createModule(sol::state_view lua)
{
    sol::table out = lua.create_table();

    out["DateFormat"] = createEnumTable<Qt::DateFormat>(lua);
    out["TimeSpec"] = createEnumTable<Qt::TimeSpec>(lua);
    out["QDateTime"] = out.new_usertype<QDateTime>(
        // clang-format off
        "QDateTime",
        sol::factories(
            // clang-format on
            [](QDate date, QTime time) -> QDateTime {
                return {date, time};
            },
            [](QDate date, QTime time, Qt::TimeSpec spec) -> QDateTime {
                return {date, time, spec};
            },
            [](QDate date, QTime time, Qt::TimeSpec spec,
               int offsetSeconds) -> QDateTime {
                return {date, time, spec, offsetSeconds};
            },
            [](QDate date, QTime time, const QTimeZone &tz) -> QDateTime {
                // since we support versions of qt < 6.7 we can't use TransitionResolution
                return {date, time, tz};
            }
            // clang-format off
        ),
        // metamethods
        sol::meta_method::to_string, &qdtMetaToString,
        //sol::meta_method::equal_to, &qdtEqual,

        // methods
#define ADD(NAME) #NAME, &QDateTime::NAME
        ADD(isNull),
        ADD(isValid),
        ADD(date),
        ADD(time),
        ADD(timeSpec),
        ADD(offsetFromUtc),
        ADD(timeRepresentation),
        ADD(timeZone),
        ADD(timeZoneAbbreviation),
        ADD(isDaylightTime),
        ADD(toMSecsSinceEpoch),
        ADD(toSecsSinceEpoch),
        ADD(setDate), // XXX: [^1] look at qt version differences
        ADD(setTime),
        ADD(setTimeSpec), // deprecated
        ADD(setTimeZone), // ^1
        ADD(setMSecsSinceEpoch),
        ADD(setSecsSinceEpoch),
        "toString", sol::overload(
            // clang-format on
            [](const QDateTime *self) -> QString {
                // default args
                return self->toString();
            },
            // clang-format off
            static_cast<QString(QDateTime::*)(Qt::DateFormat) const>(&QDateTime::toString),
            static_cast<QString(QDateTime::*)(const QString&) const>(&QDateTime::toString)
        ),

        // all nodiscard
        ADD(addDays),
        ADD(addMonths),
        ADD(addYears),
        ADD(addSecs),
        ADD(addMSecs),

        // we don't support std::chrono durations
        ADD(toTimeSpec),
        ADD(toLocalTime),
        ADD(toUTC),
        ADD(toOffsetFromUtc),
        ADD(toTimeZone),

        ADD(daysTo),
        ADD(secsTo),
        ADD(msecsTo),

        // statics
        "currentDateTime",
        sol::overload(
            static_cast<QDateTime(*)()>(&QDateTime::currentDateTime),
            static_cast<QDateTime(*)(const QTimeZone&)>(&QDateTime::currentDateTime)
        ),
        ADD(currentDateTimeUtc),

        "fromString",
        sol::overload(
            // clang-format on
            [](const QString &string) {
                // default params
                return QDateTime::fromString(string);
            },
            // clang-format off
            static_cast<QDateTime(*)(const QString&, Qt::DateFormat)>(&QDateTime::fromString)
        ),

        "fromMSecsSinceEpoch",
        sol::overload(
            static_cast<QDateTime(*)(qint64, const QTimeZone &)>(&QDateTime::fromMSecsSinceEpoch),
            static_cast<QDateTime(*)(qint64)>(&QDateTime::fromMSecsSinceEpoch),

            // this one is deprecated
            // clang-format on
            [](qint64 msecs, Qt::TimeSpec spec) {
                // default args
                return QDateTime::fromMSecsSinceEpoch(msecs, spec);
            },
            // clang-format off
            static_cast<QDateTime (*)(qint64, Qt::TimeSpec, int)>(&QDateTime::fromMSecsSinceEpoch)
        ),
        "fromSecsSinceEpoch",
        sol::overload(
            static_cast<QDateTime(*)(qint64, const QTimeZone &)>(&QDateTime::fromSecsSinceEpoch),
            static_cast<QDateTime(*)(qint64)>(&QDateTime::fromSecsSinceEpoch),

            // this one is deprecated
            // clang-format on
            [](qint64 secs, Qt::TimeSpec spec) {
                // default args
                return QDateTime::fromSecsSinceEpoch(secs, spec);
            },
            // clang-format off
            static_cast<QDateTime(*)(qint64, Qt::TimeSpec, int)>(&QDateTime::fromSecsSinceEpoch)
        ),

        ADD(currentMSecsSinceEpoch),
        ADD(currentSecsSinceEpoch)
#undef ADD
        // clang-format on
    );

    out["QDate"] = out.new_usertype<QDate>(
        "QDate",
        // clang-format off
        sol::constructors<QDate(), QDate(int, int, int)>(),
        sol::meta_function::to_string, &qdMetaToString,
#define ADD(NAME) #NAME, &QDate::NAME
#define NO_CAL_OVERLOAD(NAME) #NAME, static_cast<int(QDate::*)() const>(&QDate:: NAME)
        NO_CAL_OVERLOAD(year),
        NO_CAL_OVERLOAD(month),
        NO_CAL_OVERLOAD(day),
        NO_CAL_OVERLOAD(dayOfWeek),
        NO_CAL_OVERLOAD(dayOfYear),
        NO_CAL_OVERLOAD(daysInMonth),
        NO_CAL_OVERLOAD(daysInYear),
        //ADD(weekNumber), // TODO: pointer out
#undef NO_CAL_OVERLOAD
        "startOfDay", sol::overload(
            static_cast<QDateTime(QDate::*)(Qt::TimeSpec, int) const>(&QDate::startOfDay),
            static_cast<QDateTime(QDate::*)(const QTimeZone&) const>(&QDate::startOfDay),
            static_cast<QDateTime(QDate::*)() const>(&QDate::startOfDay)
        ),
        "endOfDay", sol::overload(
            static_cast<QDateTime(QDate::*)(Qt::TimeSpec, int) const>(&QDate::endOfDay),
            static_cast<QDateTime(QDate::*)(const QTimeZone&) const>(&QDate::endOfDay),
            static_cast<QDateTime(QDate::*)() const>(&QDate::endOfDay)
        ),
        "toString", sol::overload(
            // clang-format on
            [](const QDate *self) -> QString {
                // default args
                return self->toString();
            },
            // clang-format off
            static_cast<QString(QDate::*)(Qt::DateFormat) const>(&QDate::toString),
            static_cast<QString(QDate::*)(const QString&) const>(&QDate::toString)
            // no cal
        ),
        "setDate", static_cast<bool(QDate::*)(int, int, int)>(&QDate::setDate),
        // getDate uses pointer output, not adding it
        ADD(addDays), // nodiscard
        "addMonths", static_cast<QDate(QDate::*)(int) const>(&QDate::addMonths),
        "addYears", static_cast<QDate(QDate::*)(int) const>(&QDate::addYears),
        ADD(daysTo),
        // statics
        ADD(currentDate),
        "fromString", sol::overload(
            static_cast<QDate(*)(const QString&, Qt::DateFormat)>(&QDate::fromString),
            [](const QString& string, const QString& format, sol::variadic_args args) -> QDate {
                // clang-format on
                if (args.size() > 1)
                {
                    throw std::runtime_error(
                        "This QDate.fromString overload (string, format) takes "
                        "only one option the baseYear");
                }
                if (args.size() > 0)
                {
                    return QDate::fromString(string, format, args.get<int>(0));
                }
                return QDate::fromString(string, format);
                // clang-format off
            }
        ),
        "isValid", static_cast<bool(*)(int, int, int)>(&QDate::isValid),
        ADD(isLeapYear)
#undef ADD
        // clang-format on
    );
    // QTime
    out["QTime"] = out.new_usertype<QTime>(
        // clang-format off
        "QTime",
        sol::constructors<QTime(), QTime(int, int, int, int)>(),
        sol::meta_function::to_string, &qtMetaToString,
#define ADD(NAME) #NAME, &QTime::NAME
        ADD(fromMSecsSinceStartOfDay), // alternate constructor

        ADD(hour),
        ADD(minute),
        ADD(second),
        ADD(msec),

        "toString", sol::overload(
            // clang-format on
            [](const QTime *self) -> QString {
                // default args
                return self->toString();
            },
            // clang-format off
            static_cast<QString(QTime::*)(Qt::DateFormat) const>(&QTime::toString),
            static_cast<QString(QTime::*)(const QString&) const>(&QTime::toString)
        ),
        ADD(setHMS),
        ADD(addSecs), // nodiscard
        ADD(secsTo),
        ADD(addMSecs), // nodiscard
        ADD(msecsTo),
        ADD(msecsSinceStartOfDay),
        ADD(currentTime),
        "fromString", sol::overload(
            static_cast<QTime(*)(const QString&, Qt::DateFormat)>(&QTime::fromString),
            static_cast<QTime(*)(const QString&, const QString&)>(&QTime::fromString)
        )
#undef ADD
        // clang-format on
    );
    // TODO: QLocale
    out["QTimeZone"] = out.new_usertype<QTimeZone>(
        "QTimeZone",  //
        // clang-format off
        sol::factories(
            []() {
                return QTimeZone();
            },
            [](int offsetSeconds) {
                return QTimeZone(offsetSeconds);
            },
            [](const QString &ianaId) {
                return QTimeZone(ianaId.toLocal8Bit());
            }  //
        ),
#define ADD(NAME) #NAME, &QTimeZone::NAME
        ADD(isValid),
        ADD(timeSpec),
        ADD(fixedSecondsAheadOfUtc),
        //ADD(isUtcOrFixedOffset),
        "isUtcOrFixedOffset", sol::overload(
            static_cast<bool(*)(Qt::TimeSpec)>(&QTimeZone::isUtcOrFixedOffset),
            static_cast<bool(QTimeZone::*)() const>(&QTimeZone::isUtcOrFixedOffset)
        ),
        ADD(asBackendZone),
        "TimeType", createEnumTable<QTimeZone::TimeType>(lua),
        "NameType", createEnumTable<QTimeZone::NameType>(lua),
        // TODO: push/pop for OffsetData
        // TODO: hasAlternativeName
        ADD(id),
        // TODO: country() (QLocale needed)
        ADD(comment),
        // TODO: displayName() (QLocale needed)
        ADD(abbreviation),

        ADD(offsetFromUtc),
        ADD(standardTimeOffset),
        ADD(daylightTimeOffset),
        ADD(hasDaylightTime),
        ADD(isDaylightTime),
        ADD(offsetData),
        ADD(hasTransitions),
        // ADD(nextTransition)
        // ADD(previousTransition)
        // ADD(transitions)

        // statics
        ADD(systemTimeZoneId),
        ADD(systemTimeZone),
        ADD(utc),
        ADD(isTimeZoneIdAvailable),
        "availableTimeZoneIds",
        sol::overload(
            []() -> std::vector<QString> {
                auto tzs = QTimeZone::availableTimeZoneIds();
                std::vector<QString> out;
                for(const auto& tz :tzs) {
                    out.emplace_back(QString::fromLocal8Bit(tz));
                }
                return out;
            },
            // TODO: static_cast<QList<QByteArray>(*)(QLocale::Territory)>(&QTimeZone::availableTimeZoneIds)
            //static_cast<QList<QByteArray>(*)(int)>(&QTimeZone::availableTimeZoneIds)
            [](int offsetSeconds) -> std::vector<QString> {
                auto tzs = QTimeZone::availableTimeZoneIds(offsetSeconds);
                std::vector<QString> out;
                for(const auto& tz :tzs) {
                    out.emplace_back(QString::fromLocal8Bit(tz));
                }
                return out;
            }
        ),
        ADD(ianaIdToWindowsId),
        "windowsIdToDefaultIanaId",
        sol::overload(
            static_cast<QByteArray(*)(const QByteArray&)>(&QTimeZone::windowsIdToDefaultIanaId)
            // TODO static_cast<QByteArray(*)(const QByteArray&, QLocale::Territory)>(&QTimeZone::windowsIdToDefaultIanaId)
        ),
        "windowsIdToIanaIds",
        sol::overload(
            static_cast<QList<QByteArray>(*)(const QByteArray&)>(&QTimeZone::windowsIdToIanaIds)
            // TODO static_cast<QList<QByteArray>(*)(const QByteArray&, QLocale::Territory)>(&QTimeZone::windowsIdToIanaIds)
        )
#undef ADD
        // clang-format on
    );

    return out;
}

}  // namespace chatterino::lua::api::qt
#endif
