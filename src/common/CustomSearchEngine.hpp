// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>

namespace chatterino {

struct CustomSearchEngine {
    QString name;  // Optional display name (can be empty)
    QString url;   // Required URL template (e.g., "https://search.yahoo.com/search?p=")

    CustomSearchEngine() = default;
    CustomSearchEngine(const QString &name, const QString &url)
        : name(name)
        , url(url)
    {
    }

    bool operator==(const CustomSearchEngine &other) const
    {
        return this->name == other.name && this->url == other.url;
    }

    [[nodiscard]] QString displayName() const
    {
        return this->name.isEmpty() ? this->url : this->name;
    }
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::CustomSearchEngine> {
    static rapidjson::Value get(const chatterino::CustomSearchEngine &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.name, a);
        chatterino::rj::set(ret, "url", value.url, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::CustomSearchEngine> {
    static chatterino::CustomSearchEngine get(const rapidjson::Value &value,
                                              bool *error = nullptr)
    {
        chatterino::CustomSearchEngine engine;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return engine;
        }

        if (!chatterino::rj::getSafe(value, "url", engine.url))
        {
            PAJLADA_REPORT_ERROR(error);
            return engine;
        }

        chatterino::rj::getSafe(value, "name", engine.name);

        return engine;
    }
};

}  // namespace pajlada
