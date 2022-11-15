#include "Types.hpp"

#include "controllers/filters/parser/Tokenizer.hpp"
#include "controllers/filters/parser/expressions/Expression.hpp"

namespace filterparser {

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

PossibleType::PossibleType(QMetaType::Type t)
{
    this->types_.insert(t);
}

PossibleType::PossibleType(std::initializer_list<QMetaType::Type> t)
    : types_(t)
{
    assert(!this->types_.empty());
}

QString PossibleType::string() const
{
    if (this->types_.size() == 1)
    {
        return metaTypeToString(*this->types_.begin());
    }
    else
    {
        QStringList names;
        names.reserve(this->types_.size());
        for (QMetaType::Type t : this->types_)
        {
            names.push_back(metaTypeToString(t));
        }
        return "(" + names.join(" | ") + ")";
    }
}

bool PossibleType::operator==(QMetaType::Type t) const
{
    return this->types_.count(t) != 0;
}

bool PossibleType::operator==(const PossibleType &p) const
{
    // Check if there are any common types between the two sets
    auto i = this->types_.cbegin();
    auto j = p.types_.cbegin();
    while (i != this->types_.cend() && j != p.types_.cend())
    {
        if (*i == *j)
            return true;
        else if (*i < *j)
            ++i;
        else
            ++j;
    }
    return false;
}

bool PossibleType::operator!=(QMetaType::Type t) const
{
    return this->types_.count(t) == 0;
}

bool PossibleType::operator!=(const PossibleType &p) const
{
    return !this->operator==(p);
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

bool TypeValidator::must(bool condition, TokenType op,
                         const PossibleType &right, const Expression *wholeExp)
{
    if (!condition)
    {
        this->fail(QStringLiteral("Can't compute %1 for %2\n\nExpression: %4")
                       .arg(tokenTypeToInfoString(op), right.string(),
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

}  // namespace filterparser
