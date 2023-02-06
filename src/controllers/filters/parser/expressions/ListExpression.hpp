#pragma once

#include "controllers/filters/parser/Types.hpp"
#include "controllers/filters/parser/expressions/Expression.hpp"

namespace filterparser {

class ListExpression : public Expression
{
public:
    ListExpression(ExpressionList list);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    ExpressionList list_;
};

}  // namespace filterparser
