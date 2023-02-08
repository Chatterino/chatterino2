#include "Types.hpp"

#include "controllers/filters/parser/expressions/Expression.hpp"
#include "controllers/filters/parser/Tokenizer.hpp"

namespace filterparser {

bool isList(PossibleType typ)
{
    using T = Type;
    return typ == T::List || typ == T::MatchingSpecifier;
}

QString typeToString(Type type)
{
    using T = Type;
    switch (type)
    {
        case T::String:
            return "String";
        case T::Int:
            return "Int";
        case T::Bool:
            return "Bool";
        case T::Color:
            return "Color";
        case T::RegularExpression:
            return "RegularExpression";
        case T::List:
            return "List";
        case T::MatchingSpecifier:
            return "MatchingSpecifier";
        case T::Map:
            return "Map";
        default:
            return "Unknown";
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

PossibleType::PossibleType(Type t)
    : type_(t)
    , illTyped_({})
{
}

PossibleType::PossibleType(IllTyped illTyped)
    : type_(Type::Bool)  // arbitrary default
    , illTyped_(std::move(illTyped))
{
}

QString PossibleType::string() const
{
    if (this->well())
    {
        return typeToString(this->type_);
    }

    return "IllTyped";
}

bool PossibleType::operator==(Type t) const
{
    assert(this->well());
    return this->type_ == t;
}

bool PossibleType::operator==(const PossibleType &p) const
{
    assert(this->well());
    if (!p.well())
    {
        return false;  // Ill-type never equal
    }

    return this->type_ == p.type_;
}

bool PossibleType::operator!=(Type t) const
{
    return !this->operator==(t);
}

bool PossibleType::operator!=(const PossibleType &p) const
{
    return !this->operator==(p);
}

bool PossibleType::well() const
{
    // whether we are well-typed
    return !this->illTyped_.has_value();
}

PossibleType::operator bool() const
{
    return this->well();
}

const std::optional<IllTyped> &PossibleType::illTypedDescription() const
{
    return this->illTyped_;
}

}  // namespace filterparser
