// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/filters/lang/expressions/ValueExpression.hpp"

#include "controllers/filters/lang/Tokenizer.hpp"

namespace chatterino::filters {

ValueExpression::ValueExpression(QVariant value, TokenType type)
    : value_(std::move(value))
    , type_(type)
{
}

QVariant ValueExpression::execute(RunContext /* context */) const
{
    return this->value_;
}

PossibleType ValueExpression::synthesizeType() const
{
    switch (this->type_)
    {
        case TokenType::INT:
            return TypeClass{Type::Int};
        case TokenType::STRING:
            return TypeClass{Type::String};
        default:
            return IllTyped{this, "Invalid value type"};
    }
}

TokenType ValueExpression::type()
{
    return this->type_;
}

QString ValueExpression::debug() const
{
    return QString("Val(%1)").arg(this->value_.toString());
}

QString ValueExpression::filterString() const
{
    switch (this->type_)
    {
        case INT:
            return QString::number(this->value_.toInt());
        case STRING:
            return QString("\"%1\"").arg(
                this->value_.toString().replace("\"", "\\\""));
        default:
            return "";
    }
}

}  // namespace chatterino::filters
