#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

bool convertVariantTypes(QVariant &a, QVariant &b, int type)
{
    return a.convert(type) && b.convert(type);
}

QString tokenTypeToString(TokenType type)
{
    switch (type)
    {
        case CONTROL_START:
        case CONTROL_END:
        case BINARY_START:
        case BINARY_END:
        case UNARY_START:
        case UNARY_END:
        case MATH_START:
        case MATH_END:
        case OTHER_START:
        case NONE:
            return "<none>";
        case AND:
            return "&&";
        case OR:
            return "||";
        case LP:
            return "(";
        case RP:
            return ")";
        case PLUS:
            return "+";
        case MINUS:
            return "-";
        case MULTIPLY:
            return "*";
        case DIVIDE:
            return "/";
        case MOD:
            return "%";
        case EQ:
            return "==";
        case NEQ:
            return "!=";
        case LT:
            return "<";
        case GT:
            return ">";
        case LTE:
            return "<=";
        case GTE:
            return ">=";
        case CONTAINS:
            return "contains";
        case NOT:
            return "!";
        case STRING:
            return "<string>";
        case INT:
            return "<int>";
        case IDENTIFIER:
            return "<identifier>";
        default:
            return "<unknown>";
    }
}

// ValueExpression

ValueExpression::ValueExpression(QVariant value, TokenType type)
    : value_(value)
    , type_(type){};

QVariant ValueExpression::execute(const ContextMap &context)
{
    if (this->type_ == TokenType::IDENTIFIER)
    {
        return context.value(this->value_.toString());
    }
    return this->value_;
}

TokenType ValueExpression::type()
{
    return this->type_;
}

QString ValueExpression::debug()
{
    return this->value_.toString();
    //        return QString("(%1 %2)").arg(tokenTypeToString(this->type_),
    //                                      this->value_.toString());
}

// BinaryOperation

BinaryOperation::BinaryOperation(TokenType op, Expression *left,
                                 Expression *right)
    : op_(op)
    , left_(left)
    , right_(right)
{
}

QVariant BinaryOperation::execute(const ContextMap &context)
{
    auto left = this->left_->execute(context);
    auto right = this->right_->execute(context);
    switch (this->op_)
    {
        case PLUS:
            if (left.type() == QVariant::Type::String &&
                right.canConvert(QMetaType::QString))
            {
                return left.toString().append(right.toString());
            }
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() + right.toInt();
            }
            return 0;
        case MINUS:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() - right.toInt();
            return 0;
        case MULTIPLY:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() * right.toInt();
            return 0;
        case DIVIDE:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() / right.toInt();
            return 0;
        case MOD:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() % right.toInt();
            return 0;
        case OR:
            if (convertVariantTypes(left, right, QMetaType::Bool))
                return left.toBool() || right.toBool();
            return false;
        case AND:
            if (convertVariantTypes(left, right, QMetaType::Bool))
                return left.toBool() && right.toBool();
            return false;
        case EQ:
            return left == right;
        case NEQ:
            return left != right;
        case LT:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() < right.toInt();
            return false;
        case GT:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() > right.toInt();
            return false;
        case LTE:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() <= right.toInt();
            return false;
        case GTE:
            if (convertVariantTypes(left, right, QMetaType::Int))
                return left.toInt() >= right.toInt();
            return false;
        case CONTAINS:
            if (left.type() == QVariant::Type::StringList &&
                right.canConvert(QMetaType::QString))
            {
                return left.toStringList().contains(right.toString(),
                                                    Qt::CaseInsensitive);
            }

            if (left.type() == QVariant::Type::Map &&
                right.canConvert(QMetaType::QString))
            {
                return left.toMap().contains(right.toString());
            }

            if (left.canConvert(QMetaType::QString) &&
                right.canConvert(QMetaType::QString))
            {
                return left.toString().contains(right.toString(),
                                                Qt::CaseInsensitive);
            }

            return false;
        default:
            qDebug() << "tried binary for non-binary op:" << this->op_;
            return false;
    }
}

QString BinaryOperation::debug()
{
    return QString("(%1 %2 %3)")
        .arg(this->left_->debug(), tokenTypeToString(this->op_),
             this->right_->debug());
}

// UnaryOperation

UnaryOperation::UnaryOperation(TokenType op, Expression *right)
    : op_(op)
    , right_(right)
{
}

QVariant UnaryOperation::execute(const ContextMap &context)
{
    auto right = this->right_->execute(context);
    switch (this->op_)
    {
        case NOT:
            if (right.canConvert<bool>())
                return !right.toBool();
            return false;
        default:
            qDebug() << "tried unary for non-unary op:" << this->op_;
            return false;
    }
}

QString UnaryOperation::debug()
{
    return QString("(%1 %2)").arg(tokenTypeToString(this->op_),
                                  this->right_->debug());
}

}  // namespace filterparser
