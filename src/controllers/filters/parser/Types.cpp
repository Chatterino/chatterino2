#include "controllers/filters/parser/Types.hpp"
#include "controllers/filters/parser/Tokenizer.hpp"

namespace filterparser {

namespace {
    inline bool variantIs(const QVariant &a, QMetaType::Type type)
    {
        return static_cast<QMetaType::Type>(a.type()) == type;
    }

    inline bool variantIsNot(const QVariant &a, QMetaType::Type type)
    {
        return static_cast<QMetaType::Type>(a.type()) != type;
    }

    inline bool convertVariantTypes(QVariant &a, QVariant &b, int type)
    {
        return a.convert(type) && b.convert(type);
    }

    inline bool variantTypesMatch(QVariant &a, QVariant &b,
                                  QMetaType::Type type)
    {
        return variantIs(a, type) && variantIs(b, type);
    }
}  // namespace

QString metaTypeToString(QMetaType::Type type)
{
    using T = QMetaType;
    switch (type)
    {
        case T::QString:
            return "string";
        case T::Bool:
            return "boolean";
        case T::Int:
            return "integer";
        case T::QVariantList:
        case T::QStringList:
            return "list";
        case T::QRegExp:
            return "regular expression";
        case T::QColor:
            return "color";
        default:
            return QString::fromUtf8(QMetaType::typeName(type));
    }
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
        case MATCH:
            return "<match>";
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

TypeValidator::TypeValidator()
{
}

bool TypeValidator::must(bool condition, const QString &message)
{
    if (!condition)
    {
        this->fail(message);
    }
    return condition;
}

bool TypeValidator::must(bool condition, TokenType op, const PossibleType &left,
                         const PossibleType &right)
{
    if (!condition)
    {
        this->fail(
            QStringLiteral("Can't compute %1 for %2 and %3")
                .arg(tokenTypeToInfoString(op), left.string(), right.string()));
    }
    return condition;
}

bool TypeValidator::must(bool condition, TokenType op, const PossibleType &left,
                         const PossibleType &right, const Expression *wholeExp)
{
    if (!condition)
    {
        this->fail(
            QStringLiteral("Can't compute %1 for %2 and %3\n\nExpression: %4")
                .arg(tokenTypeToInfoString(op), left.string(), right.string(),
                     wholeExp->filterString()));
    }
    return condition;
}

void TypeValidator::fail(const QString &message)
{
    this->valid_ = false;
    this->failureMessage_ = message;
}

bool TypeValidator::valid() const
{
    return this->valid_;
}

const QString &TypeValidator::failureMessage()
{
    return this->failureMessage_;
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

PossibleType ValueExpression::returnType() const
{
    if (this->type_ == TokenType::IDENTIFIER)
    {
        auto it = validIdentifiersMap.find(this->value_.toString());
        if (it != validIdentifiersMap.end())
        {
            return it.value().type;
        }

        return Expression::returnType();  // Invalid fallback
    }
    return static_cast<QMetaType::Type>(this->value_.type());
}

bool ValueExpression::validateTypes(TypeValidator &validator) const
{
    return true;  // Nothing to do
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

// RegexExpression

RegexExpression::RegexExpression(QString regex, bool caseInsensitive)
    : regexString_(regex)
    , caseInsensitive_(caseInsensitive)
    , regex_(QRegularExpression(
          regex, caseInsensitive ? QRegularExpression::CaseInsensitiveOption
                                 : QRegularExpression::NoPatternOption)){};

QVariant RegexExpression::execute(const ContextMap &) const
{
    return this->regex_;
}

PossibleType RegexExpression::returnType() const
{
    return QMetaType::QRegularExpression;
}

bool RegexExpression::validateTypes(TypeValidator &validator) const
{
    return true;  // Nothing to do
}

QString RegexExpression::debug() const
{
    return this->regexString_;
}

QString RegexExpression::filterString() const
{
    auto s = this->regexString_;
    return QString("%1\"%2\"")
        .arg(this->caseInsensitive_ ? "ri" : "r")
        .arg(s.replace("\"", "\\\""));
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
        if (allStrings && variantIsNot(res.type(), QMetaType::QString))
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

PossibleType ListExpression::returnType() const
{
    for (const auto &exp : this->list_)
    {
        if (exp->returnType() != QMetaType::QString)
        {
            return QMetaType::QVariantList;
        }
    }

    // Every item evaluates to a string
    return QMetaType::QStringList;
}

bool ListExpression::validateTypes(TypeValidator &validator) const
{
    return true;  // Nothing to do
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
                                            Qt::CaseInsensitive);
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
                                           Qt::CaseInsensitive);
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

                    // if matched, return nth capture group. Otherwise, return false
                    if (match.hasMatch())
                        return match.captured(list.at(1).toInt());
                    else
                        return false;
                }
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

PossibleType BinaryOperation::returnType() const
{
    auto left = this->left_->returnType();
    auto right = this->right_->returnType();
    switch (this->op_)
    {
        case PLUS:
            if (left == QMetaType::QString)
            {
                return QMetaType::QString;  // String concatenation
            }
            return QMetaType::Int;
        case MINUS:
        case MULTIPLY:
        case DIVIDE:
        case MOD:
            return QMetaType::Int;
        case OR:
        case AND:
        case EQ:
        case NEQ:
        case LT:
        case GT:
        case LTE:
        case GTE:
        case CONTAINS:
        case STARTS_WITH:
        case ENDS_WITH:
            return QMetaType::Bool;
        case MATCH: {
            if (left != QMetaType::QString)
            {
                return QMetaType::Bool;
            }

            if (right == QMetaType::QRegularExpression)
            {
                return QMetaType::Bool;
            }
            else if (right == QMetaType::QVariantList)
            {
                return {QMetaType::QString, QMetaType::Bool};
            }

            return QMetaType::Bool;
        }
        default:
            return QMetaType::Bool;
    }
}

bool BinaryOperation::validateTypes(TypeValidator &validator) const
{
    if (!this->left_->validateTypes(validator) ||
        !this->right_->validateTypes(validator))
    {
        return false;
    }

    auto left = this->left_->returnType();
    auto right = this->right_->returnType();
    switch (this->op_)
    {
        case PLUS:
            if (left == QMetaType::QString)
            {
                return true;
            }
            return validator.must(
                left == QMetaType::Int && right == QMetaType::Int, this->op_,
                left, right, this);
        case MINUS:
        case MULTIPLY:
        case DIVIDE:
        case MOD:
        case LT:
        case GT:
            return validator.must(
                left == QMetaType::Int && right == QMetaType::Int, this->op_,
                left, right, this);
        case OR:
        case AND:
            return validator.must(
                left == QMetaType::Bool && right == QMetaType::Bool, this->op_,
                left, right, this);
        case EQ:
        case NEQ:
            // todo:
            // validator.must(left == right || left == QMetaType::QString,
            //                this->op_, left, right, this);
            return true;
        case LTE:
        case GTE:
            return validator.must(
                left == QMetaType::Int && right == QMetaType::Int, this->op_,
                left, right, this);
        case CONTAINS:
        case STARTS_WITH:
        case ENDS_WITH:
            return validator.must(left == QMetaType::QVariantList ||
                                      left == QMetaType::QStringList ||
                                      left == QMetaType::QString,
                                  this->op_, left, right, this);
        case MATCH: {
            if (left != QMetaType::QString)
            {
                validator.fail(
                    QStringLiteral(
                        "Can't match on type %1, only string\n\nExpression: %s")
                        .arg(left.string(), this->filterString()));
                return false;
            }

            if (right == QMetaType::QRegularExpression ||
                right == QMetaType::QVariantList)
            {
                return true;
            }

            return validator.must(false, this->op_, left, right, this);
        }
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
            case MATCH:
                return "match";
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
