#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>

namespace chatterino {

/**
 * @brief Contains the description of a channel that will be logged
 **/
class ChannelLog
{
    QString channelName_;

public:
    ChannelLog(QString channelName);

    bool operator==(const ChannelLog &other) const;

    [[nodiscard]] QString channelName() const;

    [[nodiscard]] QString toString() const;

    [[nodiscard]] static ChannelLog createEmpty();
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::ChannelLog> {
    static rapidjson::Value get(const chatterino::ChannelLog &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "channelName", value.channelName(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::ChannelLog> {
    static chatterino::ChannelLog get(const rapidjson::Value &value,
                                      bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return chatterino::ChannelLog::createEmpty();
        }

        QString channelName;

        if (!chatterino::rj::getSafe(value, "channelName", channelName))
        {
            PAJLADA_REPORT_ERROR(error);
            return chatterino::ChannelLog::createEmpty();
        }

        return {channelName};
    }
};

}  // namespace pajlada
