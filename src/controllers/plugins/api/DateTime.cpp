#include "controllers/plugins/api/DateTime.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep

#    include <QDateTime>

namespace chatterino::lua::api::datetime {

void createUserTypes(sol::table &c2)
{
    c2.new_usertype<QDateTime>(
        "DateTime", sol::no_constructor,  //

        // __tostring uses the ISO date.
        sol::meta_method::to_string,
        [](const QDateTime &self) {
            return self.toString(Qt::ISODateWithMs);
        },

        // ISO 8601 conversion.
        "from_iso_string",
        [](const QString &s) {
            return QDateTime::fromString(s, Qt::ISODateWithMs);
        },
        "to_iso_string",
        [](const QDateTime &self) {
            return self.toString(Qt::ISODateWithMs);
        },
        "to_iso_string_without_ms",
        [](const QDateTime &self) {
            return self.toString(Qt::ISODate);
        },

        // System time constructors.
        "current_local",
        [] {
            return QDateTime::currentDateTime();
        },
        "current_utc",
        [] {
            return QDateTime::currentDateTimeUtc();
        },

        // Unix (milli)seconds conversion.
        "from_unix_milliseconds",
        [](int64_t ts) {
            return QDateTime::fromMSecsSinceEpoch(ts);
        },
        "from_unix_seconds",
        [](int64_t ts) {
            return QDateTime::fromSecsSinceEpoch(ts);
        },
        "to_unix_milliseconds",
        [](const QDateTime &self) {
            return self.toMSecsSinceEpoch();
        },
        "to_unix_seconds",
        [](const QDateTime &self) {
            return self.toSecsSinceEpoch();
        }  //
    );
}

}  // namespace chatterino::lua::api::datetime

#endif
