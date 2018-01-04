#pragma once

#include <QString>
#include <pajlada/settings/serialize.hpp>

namespace pajlada {
namespace Settings {

template <>
struct Serialize<QString> {
    static rapidjson::Value get(const QString &value, rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(qPrintable(value), a);

        return ret;
    }
};

template <>
struct Deserialize<QString> {
    static QString get(const rapidjson::Value &value)
    {
        if (!value.IsString()) {
            throw std::runtime_error("Deserialized rapidjson::Value is not a string");
        }

        return value.GetString();
    }
};

}  // namespace Settings
}  // namespace pajlada
