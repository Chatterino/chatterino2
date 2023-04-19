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

struct IllTyped;

struct TypeClass {
    Type type;

    QString string() const;

    bool operator==(Type t) const;
    bool operator==(const TypeClass &t) const;
    bool operator==(const IllTyped &t) const;
    bool operator!=(Type t) const;
    bool operator!=(const TypeClass &t) const;
    bool operator!=(const IllTyped &t) const;
};

struct IllTyped {
    // Important nuance to expr:
    // During type synthesis, should an error occur and an IllTyped PossibleType be
    // returned, expr is a pointer to an Expression that exists in the Expression
    // tree that was parsed. Therefore, you cannot hold on to this pointer longer
    // than the Expression tree exists. Be careful!
    const Expression *expr;
    QString message;

    QString string() const;
};

using PossibleType = std::variant<TypeClass, IllTyped>;

inline bool isWellTyped(const PossibleType &possible)
{
    return std::holds_alternative<TypeClass>(possible);
}

inline bool isIllTyped(const PossibleType &possible)
{
    return std::holds_alternative<IllTyped>(possible);
}

QString possibleTypeToString(const PossibleType &possible);

bool isList(const PossibleType &possibleType);

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
