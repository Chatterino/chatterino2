#pragma once

#include <pajlada/signals.hpp>
#include <QList>
#include <QMap>
#include <QUuid>

#include <memory>

namespace chatterino {

class FilterRecord;
using FilterRecordPtr = std::shared_ptr<FilterRecord>;
struct Message;
class Channel;
using MessagePtr = std::shared_ptr<const Message>;
using ChannelPtr = std::shared_ptr<Channel>;

class FilterSet
{
public:
    FilterSet();
    FilterSet(const QList<QUuid> &filterIds);

    ~FilterSet();

    bool filter(const MessagePtr &m, ChannelPtr channel) const;
    const QList<QUuid> filterIds() const;

private:
    QMap<QUuid, FilterRecordPtr> filters_;
    pajlada::Signals::Connection listener_;

    void reloadFilters();
};

using FilterSetPtr = std::shared_ptr<FilterSet>;

}  // namespace chatterino
