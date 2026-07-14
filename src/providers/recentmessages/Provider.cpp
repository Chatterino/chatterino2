// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/recentmessages/Provider.hpp"

#include <QUrl>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

namespace chatterino::recentmessages {

Provider::Provider(QString url, bool enabled, QString id)
    : id_(std::move(id))
    , url_(std::move(url))
    , enabled_(enabled)
{
}

const QString &Provider::id() const
{
    return this->id_;
}

const QString &Provider::url() const
{
    return this->url_;
}

bool Provider::enabled() const
{
    return this->enabled_;
}

bool Provider::isBuiltIn() const
{
    return !this->id_.isEmpty();
}

bool Provider::isValid() const
{
    const QUrl url(this->url_.arg("channel"));
    return url.isValid() && !url.host().isEmpty() &&
           (url.scheme() == "http" || url.scheme() == "https") &&
           this->url_.contains("%1");
}

std::vector<Provider> defaultProviders()
{
    return {
        {"https://recent-messages.robotty.de/api/v2/recent-messages/%1", true,
         "robotty"},
        {"https://recentmessages.ivr.fi/api/v2/recent-messages/%1", true,
         "ivr"},
        {"https://rm.iore.tv/api/%1", true, "iore"},
        {"https://recent-messages.zneix.eu/api/v2/recent-messages/%1", true,
         "zneix"},
        {"https://rm.lilb.dev/api/v2/recent-messages/%1", true, "lilb"},
        {"https://logs.zonian.dev/rm/%1", true, "zonian"},
    };
}

std::vector<Provider> reconcileProviders(
    const std::vector<Provider> &providers,
    const std::vector<Provider> &builtInProviders)
{
    const auto findBuiltIn = [&builtInProviders](const QString &id) {
        return std::ranges::find(builtInProviders, id, &Provider::id);
    };
    const auto containsId = [](const std::vector<QString> &ids,
                               const QString &id) {
        return std::ranges::find(ids, id) != ids.end();
    };

    std::vector<QString> existingBuiltInIds;
    std::vector<Provider> result;
    for (const auto &provider : providers)
    {
        if (!provider.isBuiltIn())
        {
            result.push_back(provider);
            continue;
        }

        const auto builtIn = findBuiltIn(provider.id());
        if (builtIn == builtInProviders.end() ||
            containsId(existingBuiltInIds, provider.id()))
        {
            continue;
        }

        existingBuiltInIds.push_back(provider.id());
        result.emplace_back(builtIn->url(), provider.enabled(), builtIn->id());
    }

    std::vector<QString> defaultExistingBuiltInIds;
    for (const auto &provider : builtInProviders)
    {
        if (containsId(existingBuiltInIds, provider.id()))
        {
            defaultExistingBuiltInIds.push_back(provider.id());
        }
    }
    const bool usesDefaultOrder =
        existingBuiltInIds == defaultExistingBuiltInIds;

    // keep new built-ins in the default order unless the user has reordered
    // the existing built-ins. in that case, just append new built-ins instead.
    for (auto builtIn = builtInProviders.begin();
         builtIn != builtInProviders.end(); ++builtIn)
    {
        if (containsId(existingBuiltInIds, builtIn->id()))
        {
            continue;
        }

        if (!usesDefaultOrder)
        {
            result.push_back(*builtIn);
            continue;
        }

        auto insertion = result.end();
        for (auto next = std::next(builtIn); next != builtInProviders.end();
             ++next)
        {
            insertion = std::ranges::find(result, next->id(), &Provider::id);
            if (insertion != result.end())
            {
                break;
            }
        }
        if (insertion == result.end())
        {
            const auto lastBuiltIn = std::ranges::find_if(
                result | std::views::reverse, [](const Provider &provider) {
                    return provider.isBuiltIn();
                });
            if (lastBuiltIn != result.rend())
            {
                insertion = lastBuiltIn.base();
            }
            else
            {
                insertion = result.begin();
            }
        }
        result.insert(insertion, *builtIn);
        existingBuiltInIds.push_back(builtIn->id());
    }

    return result;
}

std::optional<Provider> nextEnabledProvider(
    const std::vector<Provider> &providers, std::size_t &index)
{
    while (index < providers.size())
    {
        const auto &provider = providers[index++];
        if (provider.enabled())
        {
            return provider;
        }
    }

    return std::nullopt;
}

}  // namespace chatterino::recentmessages
