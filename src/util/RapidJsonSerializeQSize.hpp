#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QSize>

namespace pajlada {

template <>
struct Serialize<QSize> {
    static rapidjson::Value get(const QSize &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "width", value.width(), a);
        chatterino::rj::set(ret, "height", value.height(), a);

        return ret;
    }
};

template <>
struct Deserialize<QSize> {
    static QSize get(const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return {};
        }

        int width{};
        int height{};

        if (!chatterino::rj::getSafe(value, "width", width))
        {
            PAJLADA_REPORT_ERROR(error);
            return {};
        }
        if (!chatterino::rj::getSafe(value, "height", height))
        {
            PAJLADA_REPORT_ERROR(error);
            return {};
        }

        return {width, height};
    }
};

}  // namespace pajlada
