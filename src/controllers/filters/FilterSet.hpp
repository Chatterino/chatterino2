#pragma once

#include "controllers/filters/FilterRecord.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

class FilterSet
{
public:
    FilterSet()
    {
        this->listener_ =
            getCSettings().filterRecords.delayedItemsChanged.connect([this] {
                this->reloadFilters();
            });
    }

    FilterSet(const QList<QUuid> &filterIds)
    {
        auto filters = getCSettings().filterRecords.readOnly();
        for (const auto &f : *filters)
        {
            if (filterIds.contains(f->getId()))
                this->filters_.insert(f->getId(), f);
        }

        this->listener_ =
            getCSettings().filterRecords.delayedItemsChanged.connect([this] {
                this->reloadFilters();
            });
    }

    ~FilterSet()
    {
        this->listener_.disconnect();
    }

    bool filter(const MessagePtr &m) const
    {
        if (this->filters_.size() == 0)
            return true;

        filterparser::ContextMap context = filterparser::buildContextMap(m);
        for (const auto &f : this->filters_.values())
        {
            if (!f->valid() || !f->filter(context))
                return false;
        }

        return true;
    }

    const QList<QUuid> filterIds() const
    {
        return this->filters_.keys();
    }

private:
    QMap<QUuid, FilterRecordPtr> filters_;
    pajlada::Signals::Connection listener_;

    void reloadFilters()
    {
        auto filters = getCSettings().filterRecords.readOnly();
        for (const auto &key : this->filters_.keys())
        {
            bool found = false;
            for (const auto &f : *filters)
            {
                if (f->getId() == key)
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
};

using FilterSetPtr = std::shared_ptr<FilterSet>;

}  // namespace chatterino
