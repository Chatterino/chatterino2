#pragma once

#include "controllers/filters/parser/Types.hpp"
#include "controllers/filters/parser/expressions/Expression.hpp"

namespace filterparser {

class ValueExpression : public Expression
{
public:
    ValueExpression(QVariant value, TokenType type);
    TokenType type();

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    QVariant value_;
    TokenType type_;
};

}  // namespace filterparser
