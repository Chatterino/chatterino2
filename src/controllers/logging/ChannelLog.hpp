#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>

namespace chatterino {

class ChannelLog {
public:
    QString channel;
    bool loggingEnabled;

    ChannelLog(const QString &channel, bool loggingEnabled);

    bool operator==(const ChannelLog &other) const;

    QString toString() const;

    static ChannelLog createEmpty();
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::ChannelLog> {
    static rapidjson::Value get(const chatterino::ChannelLog &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "channel", value.channel, a);
        chatterino::rj::set(ret, "loggingEnabled", value.loggingEnabled, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::ChannelLog> {
    static chatterino::ChannelLog get(const rapidjson::Value &value,
                                   bool *error = nullptr)
    {
        chatterino::ChannelLog ChannelLog = chatterino::ChannelLog::createEmpty();

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return ChannelLog;
        }

        if (!chatterino::rj::getSafe(value, "channel", ChannelLog.channel))
        {
            PAJLADA_REPORT_ERROR(error);
            return ChannelLog;
        }
        if (!chatterino::rj::getSafe(value, "loggingEnabled", ChannelLog.loggingEnabled))
        {
            PAJLADA_REPORT_ERROR(error);
            return ChannelLog;
        }

        return ChannelLog;
    }
};

}  // namespace pajlada
