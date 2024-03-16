#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Types.hpp"

namespace chatterino::filters {

class ValueExpression : public Expression
{
public:
    ValueExpression(QVariant value, TokenType type);
    TokenType type();

    QVariant execute(const ContextMap &context) const override;
    PossibleType synthesizeType(const TypingContext &context) const override;
    QString debug(const TypingContext &context) const override;
    QString filterString() const override;

private:
    QVariant value_;
    TokenType type_;
};

}  // namespace chatterino::filters
