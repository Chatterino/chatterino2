// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/serialize.hpp>
#include <QString>
#include <QStringList>

namespace pajlada {

template <typename T>
struct Serialize<QList<T>> {
    static rapidjson::Value get(const QList<T> &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value v(rapidjson::kArrayType);
        v.Reserve(value.size(), a);
        for (const auto &s : value)
        {
            v.PushBack(Serialize<T>::get(s, a), a);
        }
        return v;
    }
};

template <typename T>
struct Deserialize<QList<T>> {
    static QList<T> get(const rapidjson::Value &value, bool *error = nullptr)
    {
        QList<T> list;

        if (!value.IsArray())
        {
            PAJLADA_REPORT_ERROR(error)
            return list;
        }

        auto cArray = value.GetArray();
        list.reserve(cArray.Size());
        for (const auto &v : cArray)
        {
            list.append(Deserialize<T>::get(v, error));
        }
        return list;
    }
};

}  // namespace pajlada
