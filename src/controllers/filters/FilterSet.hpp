#pragma once

#include "controllers/filters/FilterRecord.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class FilterSet
{
public:
    FilterSet(const QList<QUuid> &filterIds)
        : filters_(QMap<QUuid, FilterRecord>())
    {
        auto filters = getCSettings().filterRecords.readOnly();
        for (const auto &f : *filters)
        {
            if (filterIds.contains(f.getId()))
                this->filters_.insert(f.getId(), f);
        }

        this->listener_ =
            getCSettings().filterRecords.delayedItemsChanged.connect(
                [this] { this->reloadFilters(); });
    }

    ~FilterSet()
    {
        this->listener_.disconnect();
    }

    bool filter(const MessagePtr &m) const
    {
        filterparser::ContextMap context = filterparser::buildContextMap(m);

        for (const auto &f : this->filters_.values())
        {
            if (!f.valid() || !f.filter(context))
                return false;
        }

        return true;
    }

    void reloadFilters()
    {
        auto filters = getCSettings().filterRecords.readOnly();
        for (const auto &key : this->filters_.keys())
        {
            bool found = false;
            for (const auto &f : *filters)
            {
                if (f.getId() == key)
                {
                    found = true;
                    this->filters_.insert(key, f);
                }
            }
            if (!found)
            {
                this->filters_.remove(key);
            }
        }
    }

    const QList<QUuid> filterIds() const
    {
        return this->filters_.keys();
    }

private:
    QMap<QUuid, FilterRecord> filters_;
    pajlada::Signals::Connection listener_;
};

}  // namespace chatterino
