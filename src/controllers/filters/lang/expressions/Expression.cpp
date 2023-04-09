#include "controllers/filters/lang/expressions/Expression.hpp"

namespace chatterino::filters {

QVariant Expression::execute(const ContextMap & /*context*/) const
{
    return false;
}

PossibleType Expression::synthesizeType(const TypingContext & /*context*/) const
{
    return IllTyped{this, "Not implemented"};
}

QString Expression::debug(const TypingContext & /*context*/) const
{
    return "";
}

QString Expression::filterString() const
{
    return "";
}

}  // namespace chatterino::filters
