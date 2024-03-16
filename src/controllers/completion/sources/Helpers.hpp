#pragma once

#include "widgets/listview/GenericListModel.hpp"

#include <QStringList>

#include <vector>

namespace chatterino::completion {

namespace {

    size_t sizeWithinLimit(size_t size, size_t limit)
    {
        if (limit == 0)
        {
            return size;
        }
        return std::min(size, limit);
    }

}  // namespace

template <typename T, typename Mapper>
void addVecToListModel(const std::vector<T> &input, GenericListModel &model,
                       size_t maxCount, Mapper mapper)
{
    const size_t count = sizeWithinLimit(input.size(), maxCount);
    model.reserve(model.rowCount() + count);

    for (size_t i = 0; i < count; ++i)
    {
        model.addItem(mapper(input[i]));
    }
}

template <typename T, typename Mapper>
void addVecToStringList(const std::vector<T> &input, QStringList &list,
                        size_t maxCount, Mapper mapper)
{
    const size_t count = sizeWithinLimit(input.size(), maxCount);
    list.reserve(list.count() + count);

    for (size_t i = 0; i < count; ++i)
    {
        list.push_back(mapper(input[i]));
    }
}

}  // namespace chatterino::completion
