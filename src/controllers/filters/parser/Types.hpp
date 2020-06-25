#pragma once

#include "messages/Message.hpp"

namespace filterparser {

using MessagePtr = std::shared_ptr<const chatterino::Message>;
using ContextMap = QMap<QString, QVariant>;

enum TokenType {
    // control
    CONTROL_START = 0,
    AND = 1,
    OR = 2,
    LP = 3,
    RP = 4,
    CONTROL_END = 9,

    // binary operator
    BINARY_START = 10,
    EQ = 11,
    NEQ = 12,
    LT = 13,
    GT = 14,
    LTE = 15,
    GTE = 16,
    CONTAINS = 17,
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

    NONE = 200
};

bool convertVariantTypes(QVariant &a, QVariant &b, int type);
QString tokenTypeToInfoString(TokenType type);

class Expression
{
public:
    virtual QVariant execute(const ContextMap &)
    {
        return false;
    }

    virtual QString debug()
    {
        return "(false)";
    }
};

class ValueExpression : public Expression
{
public:
    ValueExpression(QVariant value, TokenType type);
    TokenType type();

    QVariant execute(const ContextMap &context) override;
    QString debug() override;

private:
    QVariant value_;
    TokenType type_;
};

class BinaryOperation : public Expression
{
public:
    BinaryOperation(TokenType op, Expression *left, Expression *right);

    QVariant execute(const ContextMap &context) override;
    QString debug() override;

private:
    TokenType op_;
    Expression *left_;
    Expression *right_;
};

class UnaryOperation : public Expression
{
public:
    UnaryOperation(TokenType op, Expression *right);

    QVariant execute(const ContextMap &context) override;
    QString debug() override;

private:
    TokenType op_;
    Expression *right_;
};

}  // namespace filterparser
