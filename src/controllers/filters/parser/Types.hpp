#pragma once

#include <memory>
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

QString metaTypeToString(QMetaType::Type type);
QString tokenTypeToInfoString(TokenType type);

class PossibleType
{
public:
    PossibleType(QMetaType::Type t);
    PossibleType(std::initializer_list<QMetaType::Type> t);

    QString string() const;

    bool operator==(QMetaType::Type t) const;
    bool operator==(const PossibleType &p) const;
    bool operator!=(QMetaType::Type t) const;
    bool operator!=(const PossibleType &p) const;

private:
    std::set<QMetaType::Type> types_;
};

class TypeValidator
{
public:
    TypeValidator();

    bool must(bool condition, const QString &message);
    bool must(bool condition, TokenType op, const PossibleType &left,
              const PossibleType &right);
    bool must(bool condition, TokenType op, const PossibleType &left,
              const PossibleType &right, const Expression *wholeExp);

    void fail(const QString &message);

    bool valid() const;
    const QString &failureMessage();

private:
    bool valid_ = true;
    QString failureMessage_ = "";
};

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
