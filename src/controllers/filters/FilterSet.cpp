// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/filters/FilterSet.hpp"

#include "controllers/filters/FilterRecord.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

FilterSet::FilterSet()
{
    this->listener_ =
        getSettings()->filterRecords.delayedItemsChanged.connect([this] {
            this->reloadFilters();
        });
}

FilterSet::FilterSet(const QList<QUuid> &filterIds)
{
    auto filters = getSettings()->filterRecords.readOnly();
    for (const auto &f : *filters)
    {
        if (filterIds.contains(f->getId()))
        {
            this->filters_.insert(f->getId(), f);
        }
    }

    this->listener_ =
        getSettings()->filterRecords.delayedItemsChanged.connect([this] {
            this->reloadFilters();
        });
}

FilterSet::~FilterSet()
{
    this->listener_.disconnect();
}

bool FilterSet::filter(const MessagePtr &m, ChannelPtr channel) const
{
    if (this->filters_.size() == 0)
    {
        return true;
    }

    filters::RunContext ctx{
        .message = *m,
        .channel = channel.get(),
    };
    for (const auto &f : this->filters_.values())
    {
        if (!f->valid() || !f->filter(ctx))
        {
            return false;
        }
    }

    return true;
}

const QList<QUuid> FilterSet::filterIds() const
{
    return this->filters_.keys();
}

void FilterSet::reloadFilters()
{
    auto filters = getSettings()->filterRecords.readOnly();
    QSet<QUuid> currentKeys = this->filters_.keys().toSet();

    // Step 1: Update existing filters
    for (const auto &key : currentKeys)
    {
        bool found = false;
        for (const auto &f : *filters)
        {
            if (f->getId() == key)
            {
                found = true;
                this->filters_.insert(key, f);
                break;
            }
        }
        if (!found)
        {
            this->filters_.remove(key);
        }
    }

    // Step 2: Fix: Add new filters (missing in original code)
    for (const auto &f : *filters)
    {
        if (!this->filters_.contains(f->getId()))
        {
            this->filters_.insert(f->getId(), f);
        }
    }
}

}  // namespace chatterino
