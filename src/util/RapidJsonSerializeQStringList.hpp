// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/serialize.hpp>
#include <QString>
#include <QStringList>

namespace pajlada {

template <>
struct Serialize<QStringList> {
    static rapidjson::Value get(const QStringList &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value v(rapidjson::kArrayType);
        v.Reserve(value.size(), a);
        for (const auto &s : value)
        {
            const auto utf8 = s.toUtf8();
            v.PushBack(rapidjson::Value(utf8.data(), utf8.size(), a), a);
        }
        return v;
    }
};

template <>
struct Deserialize<QStringList> {
    static QStringList get(const rapidjson::Value &value, bool *error = nullptr)
    {
        QStringList list;

        if (!value.IsArray())
        {
            PAJLADA_REPORT_ERROR(error)
            return list;
        }

        auto cArray = value.GetArray();
        for (const auto &v : cArray)
        {
            if (!v.IsString())
            {
                PAJLADA_REPORT_ERROR(error);
                return list;
            }
            list.append(QString::fromUtf8(v.GetString(), v.GetStringLength()));
        }
        return list;
    }
};

}  // namespace pajlada
