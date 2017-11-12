#pragma once

#include <vector>

#include "widgets/split.hpp"

namespace chatterino {
namespace helper {
class SplitColumn
{
public:
    SplitColumn();

    void insert(widgets::Split *split, int index = -1);
    void remove(int index);
    double getFlex();
    void setFlex(double flex);

private:
    std::vector<widgets::Split> items;
};
}
}
