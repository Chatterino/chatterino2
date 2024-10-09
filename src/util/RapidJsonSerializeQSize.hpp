#pragma once

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
    static QSize get(const rapidjson::Value &value, bool *error)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return QSize{};
        }

        int _width;
        int _height;

        chatterino::rj::getSafe(value, "width", _width);
        chatterino::rj::getSafe(value, "height", _height);

        return QSize(_width, _height);
    }
};

}  // namespace pajlada
