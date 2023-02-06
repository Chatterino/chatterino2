#pragma once

#include "controllers/filters/parser/Types.hpp"
#include "controllers/filters/parser/expressions/Expression.hpp"

namespace filterparser {

class UnaryOperation : public Expression
{
public:
    UnaryOperation(TokenType op, ExpressionPtr right);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    TokenType op_;
    ExpressionPtr right_;
};

}  // namespace filterparser
