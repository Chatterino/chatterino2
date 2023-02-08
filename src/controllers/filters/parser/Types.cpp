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
