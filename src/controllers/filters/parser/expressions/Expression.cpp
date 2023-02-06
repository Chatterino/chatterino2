#include "Expression.hpp"

namespace filterparser {

QVariant Expression::execute(const ContextMap &) const
{
    return false;
}

PossibleType Expression::returnType() const
{
    return QMetaType::Bool;
}
bool Expression::validateTypes(TypeValidator &validator) const
{
    return true;
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
