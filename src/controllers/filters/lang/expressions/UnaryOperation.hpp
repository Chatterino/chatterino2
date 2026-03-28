// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Types.hpp"

namespace chatterino::filters {

class UnaryOperation : public Expression
{
public:
    UnaryOperation(TokenType op, ExpressionPtr right);

    QVariant execute(RunContext context) override;
    PossibleType synthesizeType() const override;
    QString debug() const override;
    QString filterString() const override;

private:
    TokenType op_;
    ExpressionPtr right_;
};

}  // namespace chatterino::filters
