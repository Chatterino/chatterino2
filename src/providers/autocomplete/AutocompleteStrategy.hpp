#pragma once

#include <QString>

#include <vector>

namespace chatterino {

template <typename T>
class AutocompleteStrategy
{
public:
    virtual ~AutocompleteStrategy() = default;

    virtual void apply(const std::vector<T> &items, std::vector<T> &output,
                       const QString &query) const = 0;
};

}  // namespace chatterino
