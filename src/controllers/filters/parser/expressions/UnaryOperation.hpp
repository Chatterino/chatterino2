#pragma once

#include "controllers/filters/parser/expressions/Expression.hpp"
#include "controllers/filters/parser/Types.hpp"

namespace chatterino::filters {

class UnaryOperation : public Expression
{
public:
    UnaryOperation(TokenType op, ExpressionPtr right);

    QVariant execute(const ContextMap &context) const override;
    PossibleType synthesizeType() const override;
    QString debug() const override;
    QString filterString() const override;

private:
    TokenType op_;
    ExpressionPtr right_;
};

}  // namespace chatterino::filters
