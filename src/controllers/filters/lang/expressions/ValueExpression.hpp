// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Types.hpp"

namespace chatterino::filters {

class ValueExpression : public Expression
{
public:
    ValueExpression(QVariant value, TokenType type);
    TokenType type();

    QVariant execute(RunContext context) override;
    PossibleType synthesizeType() const override;
    QString debug() const override;
    QString filterString() const override;

private:
    QVariant value_;
    TokenType type_;
};

}  // namespace chatterino::filters
