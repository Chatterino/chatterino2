#include "controllers/filters/lang/Types.hpp"

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Tokenizer.hpp"

namespace chatterino::filters {

bool isList(const PossibleType &possibleType)
{
    using T = Type;
    if (isIllTyped(possibleType))
    {
        return false;
    }

    auto typ = std::get<TypeClass>(possibleType);
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

QString TypeClass::string() const
{
    return typeToString(this->type);
}

bool TypeClass::operator==(Type t) const
{
    return this->type == t;
}

bool TypeClass::operator==(const TypeClass &t) const
{
    return this->type == t.type;
}

bool TypeClass::operator==(const IllTyped &t) const
{
    return false;
}

bool TypeClass::operator!=(Type t) const
{
    return !this->operator==(t);
}

bool TypeClass::operator!=(const TypeClass &t) const
{
    return !this->operator==(t);
}

bool TypeClass::operator!=(const IllTyped &t) const
{
    return true;
}

QString IllTyped::string() const
{
    return "IllTyped";
}

QString possibleTypeToString(const PossibleType &possible)
{
    if (isWellTyped(possible))
    {
        return std::get<TypeClass>(possible).string();
    }
    else
    {
        return std::get<IllTyped>(possible).string();
    }
}

}  // namespace chatterino::filters
