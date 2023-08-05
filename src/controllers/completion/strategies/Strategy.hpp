#pragma once

#include <QString>

#include <vector>

namespace chatterino::completion {

/// @brief An Strategy implements ordering and filtering of completion items in
/// response to a query.
/// @tparam T Type of items to consider
template <typename T>
class Strategy
{
public:
    virtual ~Strategy() = default;

    /// @brief Applies the strategy, taking the input items and storing the
    /// appropriate output items in the desired order.
    /// @param items Input items to consider
    /// @param output Output vector for items
    /// @param query Completion query
    virtual void apply(const std::vector<T> &items, std::vector<T> &output,
                       const QString &query) const = 0;
};

}  // namespace chatterino::completion
