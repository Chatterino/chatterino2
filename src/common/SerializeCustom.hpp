#pragma once

#include <QString>
#include <pajlada/settings/serialize.hpp>

namespace pajlada {
namespace Settings {

template <>
struct Serialize<QString> {
    static rapidjson::Value get(const QString &value, rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(value.toUtf8(), a);

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

        try {
            const char *str = value.GetString();
            auto strLen = value.GetStringLength();

            return QString::fromUtf8(str, strLen);
        } catch (const std::exception &) {
            //            int x = 5;
        } catch (...) {
            //            int y = 5;
        }

        return QString();
    }
};

}  // namespace Settings
}  // namespace pajlada
