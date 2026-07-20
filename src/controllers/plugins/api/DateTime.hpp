#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <sol/forward.hpp>

namespace chatterino::lua::api::datetime {

/* @lua-fragment

---A zoned date and time.
---@class c2.DateTime
c2.DateTime = {}

---Parse a date from an ISO 8601 string with milliseconds (yyyy-MM-ddTHH:mm:ss.zzz)
---@param str string
---@return c2.DateTime
function c2.DateTime.from_iso_string(str) end

---Format the datetime as an ISO string with milliseconds (yyyy-MM-ddTHH:mm:ss.zzz)
---@return string
function c2.DateTime:to_iso_string() end

---Format the datetime as an ISO string without milliseconds (yyyy-MM-ddTHH:mm:ss)
---@return string
function c2.DateTime:to_iso_string_without_ms() end

---Get the current datetime in the system's local time zone.
---@return c2.DateTime
function c2.DateTime.current_local() end

---Get the current datetime in the UTC time zone (00:00).
---@return c2.DateTime
function c2.DateTime.current_utc() end

---Get a datetime from a Unix timestamp (offset from 1970-01-01 00:00 UTC) in milliseconds.
---
---The returned date time will be in the local time zone.
---@param ts number
---@return c2.DateTime
function c2.DateTime.from_unix_milliseconds(ts) end

---Get a datetime from a Unix timestamp (offset from 1970-01-01 00:00 UTC) in seconds.
---
---The returned date time will be in the local time zone.
---@param ts number
---@return c2.DateTime
function c2.DateTime.from_unix_seconds(ts) end

---Convert a datetime to a Unix timestamp (offset from 1970-01-01 00:00 UTC) in milliseconds.
---@return number
function c2.DateTime:to_unix_milliseconds() end

---Convert a datetime to a Unix timestamp (offset from 1970-01-01 00:00 UTC) in seconds.
---@return number
function c2.DateTime:to_unix_seconds() end

---Check if the datetime is in the user's local/system timezone.
---@return boolean
function c2.DateTime:is_local() end

---Check if the datetime is in UTC.
---@return boolean
function c2.DateTime:is_utc() end

---Returns a copy of this datetime converted to the user's local/system timezone.
---@return c2.DateTime
function c2.DateTime:to_local() end

---Returns a copy of this datetime converted to UTC.
---@return c2.DateTime
function c2.DateTime:to_utc() end
*/

/// Creates
/// - `c2.DateTime` (QDateTime)
void createUserTypes(sol::table &c2);

}  // namespace chatterino::lua::api::datetime

#endif
