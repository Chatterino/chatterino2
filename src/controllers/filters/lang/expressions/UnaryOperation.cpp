// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/filters/lang/expressions/UnaryOperation.hpp"

namespace chatterino::filters {

UnaryOperation::UnaryOperation(TokenType op, ExpressionPtr right)
    : op_(op)
    , right_(std::move(right))
{
}

QVariant UnaryOperation::execute(RunContext context) const
{
    auto right = this->right_->execute(context);
    switch (this->op_)
    {
        case NOT:
            return right.canConvert<bool>() && !right.toBool();
        default:
            return false;
    }
}

PossibleType UnaryOperation::synthesizeType() const
{
    auto rightSyn = this->right_->synthesizeType();
    if (isIllTyped(rightSyn))
    {
        return rightSyn;
    }

    auto right = std::get<TypeClass>(rightSyn);

    switch (this->op_)
    {
        case NOT:
            if (right == Type::Bool)
            {
                return TypeClass{Type::Bool};
            }
            return IllTyped{this, "Can only negate boolean values"};
        default:
            return IllTyped{this, "Not implemented"};
    }
}

QString UnaryOperation::debug() const
{
    return QString("UnaryOp[%1](%2 : %3)")
        .arg(tokenTypeToInfoString(this->op_))
        .arg(this->right_->debug())
        .arg(possibleTypeToString(this->right_->synthesizeType()));
}

QString UnaryOperation::filterString() const
{
    const auto opText = [&]() -> QString {
        switch (this->op_)
        {
            case NOT:
                return "!";
            default:
                return "";
        }
    }();

    return QString("(%1%2)").arg(opText).arg(this->right_->filterString());
}

}  // namespace chatterino::filters
