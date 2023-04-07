#pragma once

#include <QMap>
#include <QString>
#include <QVariant>

#include <memory>
#include <set>
#include <variant>

namespace chatterino::filters {

class Expression;

enum class Type {
    String,
    Int,
    Bool,
    Color,
    RegularExpression,
    List,
    StringList,         // List of only strings
    MatchingSpecifier,  // 2-element list in {RegularExpression, Int} form
    Map
};

using ContextMap = QMap<QString, QVariant>;
using TypingContext = QMap<QString, Type>;

QString typeToString(Type type);

struct IllTyped {
    // Important nuance to expr:
    // During type synthesis, should an error occur and an IllTyped PossibleType be
    // returned, expr is a pointer to an Expression that exists in the Expression
    // tree that was parsed. Therefore, you cannot hold on to this pointer longer
    // than the Expression tree exists. Be careful!
    const Expression *expr;
    QString message;
};

class PossibleType
{
public:
    // Synthesized type
    PossibleType(Type t);
    // Ill-typed
    PossibleType(IllTyped illTyped);

    // Gets a string representation of the contained type.
    QString string() const;
    // Unwraps the underlying type. Must be well-typed.
    Type unwrap() const;

    // Requires that this is well-typed.
    bool operator==(Type t) const;
    // Requires that this and p are well-typed.
    bool operator==(const PossibleType &p) const;
    // Requires that this is well-typed.
    bool operator!=(Type t) const;
    // Requires that this and p are well-typed.
    bool operator!=(const PossibleType &p) const;

    // Whether this PossibleType is well-typed.
    bool well() const;
    operator bool() const;

    // Gets the IllTyped instance described by this PossibleType. Must be ill-typed.
    const IllTyped &illTypedDescription() const;

private:
    std::variant<Type, IllTyped> value_;
};

bool isList(const PossibleType &typ);

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

inline bool variantTypesMatch(QVariant &a, QVariant &b, QMetaType::Type type)
{
    return variantIs(a, type) && variantIs(b, type);
}

}  // namespace chatterino::filters
