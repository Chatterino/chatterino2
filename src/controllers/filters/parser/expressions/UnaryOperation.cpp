#include "UnaryOperation.hpp"

namespace filterparser {

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

PossibleType UnaryOperation::returnType() const
{
    auto right = this->right_->returnType();
    switch (this->op_)
    {
        case NOT:
            return QMetaType::Bool;
        default:
            return QMetaType::Bool;
    }
}

bool UnaryOperation::validateTypes(TypeValidator &validator) const
{
    if (!this->right_->validateTypes(validator))
    {
        return false;
    }

    auto right = this->right_->returnType();
    switch (this->op_)
    {
        case NOT:
            return right == QMetaType::Bool;
        default:
            return false;
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

}  // namespace filterparser
