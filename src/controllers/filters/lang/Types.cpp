#include "Types.hpp"

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Tokenizer.hpp"

namespace chatterino::filters {

bool isList(const PossibleType &possibleType)
{
    using T = Type;
    if (!possibleType.well())
    {
        return false;
    }

    T typ = possibleType.unwrap();
    return typ == T::List || typ == T::StringList ||
           typ == T::MatchingSpecifier;
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
        case T::StringList:
            return "StringList";
        case T::MatchingSpecifier:
            return "MatchingSpecifier";
        case T::Map:
            return "Map";
        default:
            return "Unknown";
    }
}

PossibleType::PossibleType(Type t)
    : value_(t)
{
}

PossibleType::PossibleType(IllTyped illTyped)
    : value_(std::move(illTyped))
{
}

QString PossibleType::string() const
{
    if (this->well())
    {
        return typeToString(this->unwrap());
    }

    return "IllTyped";
}

Type PossibleType::unwrap() const
{
    assert(this->well());
    return std::get<Type>(this->value_);
}

bool PossibleType::operator==(Type t) const
{
    return this->unwrap() == t;
}

bool PossibleType::operator==(const PossibleType &p) const
{
    if (!this->well() || !p.well())
    {
        return false;  // Ill-type never equal
    }

    return this->unwrap() == p.unwrap();
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
    return std::holds_alternative<Type>(this->value_);
}

PossibleType::operator bool() const
{
    return this->well();
}

const IllTyped &PossibleType::illTypedDescription() const
{
    assert(!this->well());
    return std::get<IllTyped>(this->value_);
}

}  // namespace chatterino::filters
