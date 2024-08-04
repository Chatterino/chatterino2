#include "controllers/filters/lang/expressions/BinaryOperation.hpp"

#include <QRegularExpression>

namespace {

/// Loosely compares `lhs` with `rhs`.
/// This attempts to convert both variants to a common type if they're not equal.
bool looselyCompareVariants(QVariant &lhs, QVariant &rhs)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6 and later don't convert types as much as Qt 5 did when comparing.
    //
    // Based on QVariant::cmp from Qt 5.15
    // https://github.com/qt/qtbase/blob/29400a683f96867133b28299c0d0bd6bcf40df35/src/corelib/kernel/qvariant.cpp#L4039-L4071
    if (lhs.metaType() != rhs.metaType())
    {
        if (rhs.canConvert(lhs.metaType()))
        {
            if (!rhs.convert(lhs.metaType()))
            {
                return false;
            }
        }
        else
        {
            // try the opposite conversion, it might work
            qSwap(lhs, rhs);
            if (!rhs.convert(lhs.metaType()))
            {
                return false;
            }
        }
    }
#endif

    return lhs == rhs;
}

}  // namespace

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
            if (variantIs(left, QMetaType::QString) &&
                right.canConvert<QString>())
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
            {
                return left.toInt() - right.toInt();
            }
            return 0;
        case MULTIPLY:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() * right.toInt();
            }
            return 0;
        case DIVIDE:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() / right.toInt();
            }
            return 0;
        case MOD:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() % right.toInt();
            }
            return 0;
        case OR:
            if (convertVariantTypes(left, right, QMetaType::Bool))
            {
                return left.toBool() || right.toBool();
            }
            return false;
        case AND:
            if (convertVariantTypes(left, right, QMetaType::Bool))
            {
                return left.toBool() && right.toBool();
            }
            return false;
        case EQ:
            if (variantTypesMatch(left, right, QMetaType::QString))
            {
                return left.toString().compare(right.toString(),
                                               Qt::CaseInsensitive) == 0;
            }
            return looselyCompareVariants(left, right);
        case NEQ:
            if (variantTypesMatch(left, right, QMetaType::QString))
            {
                return left.toString().compare(right.toString(),
                                               Qt::CaseInsensitive) != 0;
            }
            return !looselyCompareVariants(left, right);
        case LT:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() < right.toInt();
            }
            return false;
        case GT:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() > right.toInt();
            }
            return false;
        case LTE:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() <= right.toInt();
            }
            return false;
        case GTE:
            if (convertVariantTypes(left, right, QMetaType::Int))
            {
                return left.toInt() >= right.toInt();
            }
            return false;
        case CONTAINS:
            if (variantIs(left, QMetaType::QStringList) &&
                right.canConvert<QString>())
            {
                return left.toStringList().contains(right.toString(),
                                                    Qt::CaseInsensitive);
            }

            if (variantIs(left, QMetaType::QVariantMap) &&
                right.canConvert<QString>())
            {
                return left.toMap().contains(right.toString());
            }

            if (variantIs(left, QMetaType::QVariantList))
            {
                return left.toList().contains(right);
            }

            if (left.canConvert<QString>() && right.canConvert<QString>())
            {
                return left.toString().contains(right.toString(),
                                                Qt::CaseInsensitive);
            }

            return false;
        case STARTS_WITH:
            if (variantIs(left, QMetaType::QStringList) &&
                right.canConvert<QString>())
            {
                auto list = left.toStringList();
                return !list.isEmpty() &&
                       list.first().compare(right.toString(),
                                            Qt::CaseInsensitive) == 0;
            }

            if (variantIs(left, QMetaType::QVariantList))
            {
                return left.toList().startsWith(right);
            }

            if (left.canConvert<QString>() && right.canConvert<QString>())
            {
                return left.toString().startsWith(right.toString(),
                                                  Qt::CaseInsensitive);
            }

            return false;

        case ENDS_WITH:
            if (variantIs(left, QMetaType::QStringList) &&
                right.canConvert<QString>())
            {
                auto list = left.toStringList();
                return !list.isEmpty() &&
                       list.last().compare(right.toString(),
                                           Qt::CaseInsensitive) == 0;
            }

            if (variantIs(left, QMetaType::QVariantList))
            {
                return left.toList().endsWith(right);
            }

            if (left.canConvert<QString>() && right.canConvert<QString>())
            {
                return left.toString().endsWith(right.toString(),
                                                Qt::CaseInsensitive);
            }

            return false;
        case MATCH: {
            if (!left.canConvert<QString>())
            {
                return false;
            }

            auto matching = left.toString();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            switch (static_cast<QMetaType::Type>(right.typeId()))
#else
            switch (static_cast<QMetaType::Type>(right.type()))
#endif
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
                    {
                        return false;
                    }

                    // list must be a regular expression and an int
                    if (variantIsNot(list.at(0),
                                     QMetaType::QRegularExpression) ||
                        variantIsNot(list.at(1), QMetaType::Int))
                    {
                        return false;
                    }

                    auto match =
                        list.at(0).toRegularExpression().match(matching);

                    // if matched, return nth capture group. Otherwise, return ""
                    if (match.hasMatch())
                    {
                        return match.captured(list.at(1).toInt());
                    }
                    else
                    {
                        return "";
                    }
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
            {
                return TypeClass{Type::String};  // String concatenation
            }
            else if (left == Type::Int && right == Type::Int)
            {
                return TypeClass{Type::Int};
            }

            return IllTyped{this, "Can only add Ints or concatenate a String"};
        case MINUS:
        case MULTIPLY:
        case DIVIDE:
        case MOD:
            if (left == Type::Int && right == Type::Int)
            {
                return TypeClass{Type::Int};
            }

            return IllTyped{this, "Can only perform operation with Ints"};
        case OR:
        case AND:
            if (left == Type::Bool && right == Type::Bool)
            {
                return TypeClass{Type::Bool};
            }

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
            {
                return TypeClass{Type::Bool};
            }

            return IllTyped{this, "Can only perform comparisons with Ints"};
        case STARTS_WITH:
        case ENDS_WITH:
            if (isList(left))
            {
                return TypeClass{Type::Bool};
            }
            if (left == Type::String && right == Type::String)
            {
                return TypeClass{Type::Bool};
            }

            return IllTyped{
                this,
                "Can only perform starts/ends with a List or two Strings"};
        case CONTAINS:
            if (isList(left) || left == Type::Map)
            {
                return TypeClass{Type::Bool};
            }
            if (left == Type::String && right == Type::String)
            {
                return TypeClass{Type::Bool};
            }

            return IllTyped{
                this,
                "Can only perform contains with a List, a Map, or two Strings"};
        case MATCH: {
            if (left != Type::String)
            {
                return IllTyped{this,
                                "Left argument of match must be a String"};
            }

            if (right == Type::RegularExpression)
            {
                return TypeClass{Type::Bool};
            }
            if (right == Type::MatchingSpecifier)
            {  // group capturing
                return TypeClass{Type::String};
            }

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
