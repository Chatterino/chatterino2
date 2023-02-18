#include "UnaryOperation.hpp"

namespace chatterino::filters {

UnaryOperation::UnaryOperation(TokenType op, ExpressionPtr right)
    : op_(op)
    , right_(std::move(right))
{
}

QVariant UnaryOperation::execute(const ContextMap &context) const
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

PossibleType UnaryOperation::synthesizeType(const TypingContext &context) const
{
    auto right = this->right_->synthesizeType(context);
    if (!right)
    {
        return right;
    }

    switch (this->op_)
    {
        case NOT:
            if (right == Type::Bool)
            {
                return Type::Bool;
            }
            return IllTyped{this, "Can only negate boolean values"};
        default:
            return IllTyped{this, "Not implemented"};
    }
}

QString UnaryOperation::debug(const TypingContext &context) const
{
    return QString("UnaryOp[%1](%2 : %3)")
        .arg(tokenTypeToInfoString(this->op_))
        .arg(this->right_->debug(context))
        .arg(this->right_->synthesizeType(context).string());
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
