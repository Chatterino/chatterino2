#include "Expression.hpp"

namespace filterparser {

QVariant Expression::execute(const ContextMap &) const
{
    return false;
}

PossibleType Expression::synthesizeType() const
{
    return IllTyped{this, "Not implemented"};
}

QString Expression::debug() const
{
    return "(false)";
}

QString Expression::filterString() const
{
    return "";
}

}  // namespace filterparser
