#pragma once

#include "widgets/splits/Split.hpp"

#include <vector>

namespace chatterino {

class SplitColumn
{
public:
    SplitColumn() = default;

    void insert(Split *split, int index = -1);
    void remove(int index);
    double getFlex();
    void setFlex(double flex);

private:
    std::vector<Split> items;
};

}  // namespace chatterino
