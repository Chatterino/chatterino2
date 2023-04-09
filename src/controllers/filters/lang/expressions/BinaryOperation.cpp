#include "controllers/filters/lang/expressions/BinaryOperation.hpp"

#include <QRegularExpression>

namespace chatterino::filters {

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
            if (static_cast<QMetaType::Type>(left.type()) ==
                    QMetaType::QString &&
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
            if (variantTypesMatch(left, right, QMetaType::QString))
            {
                return left.toString().compare(right.toString(),
                                               Qt::CaseInsensitive) == 0;
            }
            return left == right;
        case NEQ:
            if (variantTypesMatch(left, right, QMetaType::QString))
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
            if (variantIs(left, QMetaType::QStringList) &&
                right.canConvert(QMetaType::QString))
            {
                return left.toStringList().contains(right.toString(),
                                                    Qt::CaseInsensitive);
            }

            if (variantIs(left.type(), QMetaType::QVariantMap) &&
                right.canConvert(QMetaType::QString))
            {
                return left.toMap().contains(right.toString());
            }

            if (variantIs(left.type(), QMetaType::QVariantList))
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
            if (variantIs(left.type(), QMetaType::QStringList) &&
                right.canConvert(QMetaType::QString))
            {
                auto list = left.toStringList();
                return !list.isEmpty() &&
                       list.first().compare(right.toString(),
                                            Qt::CaseInsensitive) == 0;
            }

            if (variantIs(left.type(), QMetaType::QVariantList))
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
            if (variantIs(left.type(), QMetaType::QStringList) &&
                right.canConvert(QMetaType::QString))
            {
                auto list = left.toStringList();
                return !list.isEmpty() &&
                       list.last().compare(right.toString(),
                                           Qt::CaseInsensitive) == 0;
            }

            if (variantIs(left.type(), QMetaType::QVariantList))
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
        case MATCH: {
            if (!left.canConvert(QMetaType::QString))
            {
                return false;
            }

            auto matching = left.toString();

            switch (static_cast<QMetaType::Type>(right.type()))
            {
                case QMetaType::QRegularExpression: {
                    return right.toRegularExpression()
                        .match(matching)
                        .hasMatch();
                }
                case QMetaType::QVariantList: {
                    auto list = right.toList();

                    // list must be two items
                    if (list.size() != 2)
                        return false;

                    // list must be a regular expression and an int
                    if (variantIsNot(list.at(0),
                                     QMetaType::QRegularExpression) ||
                        variantIsNot(list.at(1), QMetaType::Int))
                        return false;

                    auto match =
                        list.at(0).toRegularExpression().match(matching);

                    // if matched, return nth capture group. Otherwise, return ""
                    if (match.hasMatch())
                        return match.captured(list.at(1).toInt());
                    else
                        return "";
                }
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

PossibleType BinaryOperation::synthesizeType(const TypingContext &context) const
{
    auto leftSyn = this->left_->synthesizeType(context);
    auto rightSyn = this->right_->synthesizeType(context);

    // Return if either operand is ill-typed
    if (isIllTyped(leftSyn))
    {
        return leftSyn;
    }
    else if (isIllTyped(rightSyn))
    {
        return rightSyn;
    }

    auto left = std::get<TypeClass>(leftSyn);
    auto right = std::get<TypeClass>(rightSyn);

    switch (this->op_)
    {
        case PLUS:
            if (left == Type::String)
                return TypeClass{Type::String};  // String concatenation
            else if (left == Type::Int && right == Type::Int)
                return TypeClass{Type::Int};

            return IllTyped{this, "Can only add Ints or concatenate a String"};
        case MINUS:
        case MULTIPLY:
        case DIVIDE:
        case MOD:
            if (left == Type::Int && right == Type::Int)
                return TypeClass{Type::Int};

            return IllTyped{this, "Can only perform operation with Ints"};
        case OR:
        case AND:
            if (left == Type::Bool && right == Type::Bool)
                return TypeClass{Type::Bool};

            return IllTyped{this,
                            "Can only perform logical operations with Bools"};
        case EQ:
        case NEQ:
            // equals/not equals always produces a valid output
            return TypeClass{Type::Bool};
        case LT:
        case GT:
        case LTE:
        case GTE:
            if (left == Type::Int && right == Type::Int)
                return TypeClass{Type::Bool};

            return IllTyped{this, "Can only perform comparisons with Ints"};
        case STARTS_WITH:
        case ENDS_WITH:
            if (isList(left))
                return TypeClass{Type::Bool};
            if (left == Type::String && right == Type::String)
                return TypeClass{Type::Bool};

            return IllTyped{
                this,
                "Can only perform starts/ends with a List or two Strings"};
        case CONTAINS:
            if (isList(left) || left == Type::Map)
                return TypeClass{Type::Bool};
            if (left == Type::String && right == Type::String)
                return TypeClass{Type::Bool};

            return IllTyped{
                this,
                "Can only perform contains with a List, a Map, or two Strings"};
        case MATCH: {
            if (left != Type::String)
                return IllTyped{this,
                                "Left argument of match must be a String"};

            if (right == Type::RegularExpression)
                return TypeClass{Type::Bool};
            if (right == Type::MatchingSpecifier)  // group capturing
                return TypeClass{Type::String};

            return IllTyped{this, "Can only match on a RegularExpression or a "
                                  "MatchingSpecifier"};
        }
        default:
            return IllTyped{this, "Not implemented"};
    }
}

QString BinaryOperation::debug(const TypingContext &context) const
{
    return QString("BinaryOp[%1](%2 : %3, %4 : %5)")
        .arg(tokenTypeToInfoString(this->op_))
        .arg(this->left_->debug(context))
        .arg(possibleTypeToString(this->left_->synthesizeType(context)))
        .arg(this->right_->debug(context))
        .arg(possibleTypeToString(this->right_->synthesizeType(context)));
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
            case MATCH:
                return "match";
            default:
                return "";
        }
    }();

    return QString("(%1 %2 %3)")
        .arg(this->left_->filterString())
        .arg(opText)
        .arg(this->right_->filterString());
}

}  // namespace chatterino::filters
