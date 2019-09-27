#pragma once

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QString>
#include <pajlada/serialize.hpp>

namespace chatterino {

class TimeoutButton
{
public:
    TimeoutButton(const int &duration, const QString &unit);

    int getDuration() const;
    QString getDurationString() const;
    QString getUnit() const;
    int getTimeoutDuration() const;

private:
    int duration_;
    QString unit_;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::TimeoutButton> {
    static rapidjson::Value get(const chatterino::TimeoutButton &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "duration", value.getDuration(), a);
        chatterino::rj::set(ret, "unit", value.getUnit(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::TimeoutButton> {
    static chatterino::TimeoutButton get(const rapidjson::Value &value)
    {
        if (!value.IsObject())
        {
            return chatterino::TimeoutButton(int(), QString());
        }

        int _duration;
        QString _unit;

        chatterino::rj::getSafe(value, "duration", _duration);
        chatterino::rj::getSafe(value, "unit", _unit);

        return chatterino::TimeoutButton(_duration, _unit);
    }
};

}  // namespace pajlada
