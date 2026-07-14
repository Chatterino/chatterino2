// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>

#include <cstddef>
#include <optional>
#include <vector>

namespace chatterino::recentmessages {

class Provider
{
public:
    // built-in providers have a stable ID so their URLs can be updated across
    // releases. custom providers have an empty ID.
    Provider(QString url, bool enabled, QString id = {});

    [[nodiscard]] const QString &id() const;
    [[nodiscard]] const QString &url() const;
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] bool isBuiltIn() const;
    [[nodiscard]] bool isValid() const;

    bool operator==(const Provider &other) const = default;

private:
    QString id_;
    QString url_;
    bool enabled_;
};

std::vector<Provider> defaultProviders();
// updates the built-in providers in a list while preserving
// any customizations
std::vector<Provider> reconcileProviders(
    const std::vector<Provider> &providers,
    const std::vector<Provider> &builtInProviders);
std::optional<Provider> nextEnabledProvider(
    const std::vector<Provider> &providers, std::size_t &index);

}  // namespace chatterino::recentmessages

namespace pajlada {

template <>
struct Serialize<chatterino::recentmessages::Provider> {
    static rapidjson::Value get(
        const chatterino::recentmessages::Provider &value,
        rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "id", value.id(), a);
        chatterino::rj::set(ret, "url", value.url(), a);
        chatterino::rj::set(ret, "enabled", value.enabled(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::recentmessages::Provider> {
    static chatterino::recentmessages::Provider get(
        const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return {{}, false};
        }

        QString id;
        QString url;
        bool enabled{};

        if (!chatterino::rj::getSafe(value, "id", id) ||
            !chatterino::rj::getSafe(value, "url", url) ||
            !chatterino::rj::getSafe(value, "enabled", enabled))
        {
            PAJLADA_REPORT_ERROR(error);
            return {{}, false};
        }

        return {url, enabled, id};
    }
};

}  // namespace pajlada
