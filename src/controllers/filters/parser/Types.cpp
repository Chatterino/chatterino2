#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

bool convertVariantTypes(QVariant &a, QVariant &b, int type)
{
    return a.convert(type) && b.convert(type);
}

bool variantTypesMatch(QVariant &a, QVariant &b, QVariant::Type type)
{
    return a.type() == type && b.type() == type;
}

QString tokenTypeToInfoString(TokenType type)
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
            return "<unknown>";
        case AND:
            return "<and>";
        case OR:
            return "<or>";
        case LP:
            return "<left parenthesis>";
        case RP:
            return "<right parenthesis>";
        case LIST_START:
            return "<list start>";
        case LIST_END:
            return "<list end>";
        case COMMA:
            return "<comma>";
        case PLUS:
            return "<plus>";
        case MINUS:
            return "<minus>";
        case MULTIPLY:
            return "<multiply>";
        case DIVIDE:
            return "<divide>";
        case MOD:
            return "<modulus>";
        case EQ:
            return "<equals>";
        case NEQ:
            return "<not equals>";
        case LT:
            return "<less than>";
        case GT:
            return "<greater than>";
        case LTE:
            return "<less than equal>";
        case GTE:
            return "<greater than equal>";
        case CONTAINS:
            return "<contains>";
        case STARTS_WITH:
            return "<starts with>";
        case ENDS_WITH:
            return "<ends with>";
        case NOT:
            return "<not>";
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

QVariant ValueExpression::execute(const ContextMap &context) const
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

QString ValueExpression::debug() const
{
    return this->value_.toString();
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
        case IDENTIFIER:
            return this->value_.toString();
        default:
            return "";
    }
}

// ListExpression

ListExpression::ListExpression(ExpressionList list)
    : list_(std::move(list)){};

QVariant ListExpression::execute(const ContextMap &context) const
{
    QList<QVariant> results;
    bool allStrings = true;
    for (const auto &exp : this->list_)
    {
        auto res = exp->execute(context);
        if (allStrings && res.type() != QVariant::Type::String)
        {
            allStrings = false;
        }
        results.append(res);
    }

    // if everything is a string return a QStringList for case-insensitive comparison
    if (allStrings)
    {
        QStringList strings;
        strings.reserve(results.size());
        for (const auto &val : results)
        {
            strings << val.toString();
        }
        return strings;
    }
    else
    {
        return results;
    }
}

QString ListExpression::debug() const
{
    QStringList debugs;
    for (const auto &exp : this->list_)
    {
        debugs.append(exp->debug());
    }
    return QString("{%1}").arg(debugs.join(", "));
}

QString ListExpression::filterString() const
{
    QStringList strings;
    for (const auto &exp : this->list_)
    {
        strings.append(QString("(%1)").arg(exp->filterString()));
    }
    return QString("{%1}").arg(strings.join(", "));
}

// BinaryOperation

BinaryOperation::BinaryOperation(TokenType op, ExpressionPtr left,
                                 ExpressionPtr right)
    : op_(op)
    , left_(std::move(left))
    , right_(std::move(right))
{
}

QVariant BinaryOperation::execute(const ContextMap &context) const
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
            if (variantTypesMatch(left, right, QVariant::Type::String))
            {
                return left.toString().compare(right.toString(),
                                               Qt::CaseInsensitive) == 0;
            }
            return left == right;
        case NEQ:
            if (variantTypesMatch(left, right, QVariant::Type::String))
            {
                return left.toString().compare(right.toString(),
                                               Qt::CaseInsensitive) != 0;
            }
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

            if (left.type() == QVariant::Type::List)
            {
                return left.toList().contains(right);
            }

            if (left.canConvert(QMetaType::QString) &&
                right.canConvert(QMetaType::QString))
            {
                return left.toString().contains(right.toString(),
                                                Qt::CaseInsensitive);
            }

            return false;
        case STARTS_WITH:
            if (left.type() == QVariant::Type::StringList &&
                right.canConvert(QMetaType::QString))
            {
                auto list = left.toStringList();
                return !list.isEmpty() &&
                       list.first().compare(right.toString(),
                                            Qt::CaseInsensitive);
            }

            if (left.type() == QVariant::Type::List)
            {
                return left.toList().startsWith(right);
            }

            if (left.canConvert(QMetaType::QString) &&
                right.canConvert(QMetaType::QString))
            {
                return left.toString().startsWith(right.toString(),
                                                  Qt::CaseInsensitive);
            }

            return false;

        case ENDS_WITH:
            if (left.type() == QVariant::Type::StringList &&
                right.canConvert(QMetaType::QString))
            {
                auto list = left.toStringList();
                return !list.isEmpty() &&
                       list.last().compare(right.toString(),
                                           Qt::CaseInsensitive);
            }

            if (left.type() == QVariant::Type::List)
            {
                return left.toList().endsWith(right);
            }

            if (left.canConvert(QMetaType::QString) &&
                right.canConvert(QMetaType::QString))
            {
                return left.toString().endsWith(right.toString(),
                                                Qt::CaseInsensitive);
            }

            return false;
        default:
            return false;
    }
}

QString BinaryOperation::debug() const
{
    return QString("(%1 %2 %3)")
        .arg(this->left_->debug(), tokenTypeToInfoString(this->op_),
             this->right_->debug());
}

QString BinaryOperation::filterString() const
{
    const auto opText = [&]() -> QString {
        switch (this->op_)
        {
            case AND:
                return "&&";
            case OR:
                return "||";
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
            case STARTS_WITH:
                return "startswith";
            case ENDS_WITH:
                return "endswith";
            default:
                return QString();
        }
    }();

    return QString("(%1) %2 (%3)")
        .arg(this->left_->filterString())
        .arg(opText)
        .arg(this->right_->filterString());
}

// UnaryOperation

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
