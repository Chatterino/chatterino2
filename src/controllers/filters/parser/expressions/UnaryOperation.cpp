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
            if (right.canConvert<bool>())
                return !right.toBool();
            return false;
        default:
            return false;
    }
}

PossibleType UnaryOperation::synthesizeType() const
{
    auto right = this->right_->synthesizeType();
    if (!right)
    {
        return right;
    }

    switch (this->op_)
    {
        case NOT:
            if (right == Type::Bool)
                return Type::Bool;
            return IllTyped{this, "Can only negate boolean values"};
        default:
            return IllTyped{this, "Not implemented"};
    }
}

QString UnaryOperation::debug() const
{
    return QString("(%1 %2)").arg(tokenTypeToInfoString(this->op_),
                                  this->right_->debug());
}

QString UnaryOperation::filterString() const
{
    const auto opText = [&]() -> QString {
        switch (this->op_)
        {
            case NOT:
                return "!";
            default:
                return QString();
        }
    }();

    return QString("%1(%2)").arg(opText).arg(this->right_->filterString());
}

}  // namespace chatterino::filters
