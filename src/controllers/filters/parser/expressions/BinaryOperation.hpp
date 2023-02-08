#pragma once

#include "controllers/filters/parser/expressions/Expression.hpp"
#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

class BinaryOperation : public Expression
{
public:
    BinaryOperation(TokenType op, ExpressionPtr left, ExpressionPtr right);

    QVariant execute(const ContextMap &context) const override;
    PossibleType synthesizeType() const override;
    QString debug() const override;
    QString filterString() const override;

private:
    TokenType op_;
    ExpressionPtr left_;
    ExpressionPtr right_;
};

}  // namespace filterparser
