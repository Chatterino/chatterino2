#pragma once

#include <QString>
#include <QVariant>

#include <memory>
#include <optional>
#include <set>

namespace chatterino {

struct Message;

}

namespace filterparser {

class Expression;

using MessagePtr = std::shared_ptr<const chatterino::Message>;
using ContextMap = QMap<QString, QVariant>;

enum TokenType {
    // control
    CONTROL_START = 0,
    AND = 1,
    OR = 2,
    LP = 3,
    RP = 4,
    LIST_START = 5,
    LIST_END = 6,
    COMMA = 7,
    CONTROL_END = 19,

    // binary operator
    BINARY_START = 20,
    EQ = 21,
    NEQ = 22,
    LT = 23,
    GT = 24,
    LTE = 25,
    GTE = 26,
    CONTAINS = 27,
    STARTS_WITH = 28,
    ENDS_WITH = 29,
    MATCH = 30,
    BINARY_END = 49,

    // unary operator
    UNARY_START = 50,
    NOT = 51,
    UNARY_END = 99,

    // math operators
    MATH_START = 100,
    PLUS = 101,
    MINUS = 102,
    MULTIPLY = 103,
    DIVIDE = 104,
    MOD = 105,
    MATH_END = 149,

    // other types
    OTHER_START = 150,
    STRING = 151,
    INT = 152,
    IDENTIFIER = 153,
    REGULAR_EXPRESSION = 154,

    NONE = 200
};

QString tokenTypeToInfoString(TokenType type);

enum class Type {
    String,
    Int,
    Bool,
    Color,
    RegularExpression,
    List,
    MatchingSpecifier,  // 2-element list in {RegularExpression, Int} form
    Map
};

struct IllTyped {
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

    QString string() const;

    bool operator==(Type t) const;
    bool operator==(const PossibleType &p) const;
    bool operator!=(Type t) const;
    bool operator!=(const PossibleType &p) const;

    bool well() const;
    operator bool() const;

    const std::optional<IllTyped> &illTypedDescription() const;

private:
    Type type_;
    std::optional<IllTyped> illTyped_;
};

bool isList(PossibleType typ);

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

}  // namespace filterparser
