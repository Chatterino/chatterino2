#pragma once

#include "widgets/Split.hpp"

#include <vector>

namespace chatterino {
namespace helper {

class SplitColumn
{
public:
    SplitColumn() = default;

    void insert(widgets::Split *split, int index = -1);
    void remove(int index);
    double getFlex();
    void setFlex(double flex);

private:
    std::vector<widgets::Split> items;
};

}  // namespace helper
}  // namespace chatterino
