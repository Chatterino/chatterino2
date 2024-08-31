#pragma once

#include <pajlada/serialize.hpp>
#include <QString>

namespace pajlada {

template <>
struct Serialize<QString> {
    static rapidjson::Value get(const QString &value,
                                rapidjson::Document::AllocatorType &a)
    {
        return rapidjson::Value(value.toUtf8(), a);
    }
};

template <>
struct Deserialize<QString> {
    static QString get(const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsString())
        {
            PAJLADA_REPORT_ERROR(error)
            return QString{};
        }

        try
        {
            return QString::fromUtf8(value.GetString(),
                                     value.GetStringLength());
        }
        catch (const std::exception &)
        {
            //            int x = 5;
        }
        catch (...)
        {
            //            int y = 5;
        }

        return QString{};
    }
};

}  // namespace pajlada
