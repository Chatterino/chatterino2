#pragma once

#import "messages/Message.hpp"

namespace filterparser {

namespace {
    bool convertTypes(QVariant &a, QVariant &b, int type)
    {
        return a.convert(type) && b.convert(type);
    }
}  // namespace

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
    PLUS = 11,
    MINUS = 12,
    MULTIPLY = 13,
    DIVIDE = 14,
    MOD = 15,
    EQ = 16,
    NEQ = 17,
    LT = 18,
    GT = 19,
    LTE = 20,
    GTE = 21,
    CONTAINS = 22,
    BINARY_END = 49,

    // unary operator
    UNARY_START = 50,
    NOT = 51,
    UNARY_END = 99,

    // other types
    OTHER_START = 100,
    STRING = 101,
    INT = 102,
    IDENTIFIER = 103,

    NONE = 200
};

enum BinaryOperator {
    And = 1,
    Or = 2,

    Plus = 11,
    Minus = 12,
    Multiply = 13,
    Divide = 14,
    Modulus = 15,

    Equals = 16,
    NotEquals = 17,
    LessThan = 18,
    GreaterThan = 19,
    LessThanEqual = 20,
    GreaterThanEqual = 21,
    Contains = 22
};

enum UnaryOperator { Not = 51 };

class Expression
{
public:
    virtual QVariant execute(const ContextMap &context)
    {
        return false;
    }

    virtual QString debug()
    {
        return "Expression(false)";
    }
};

class ValueExpression : public Expression
{
public:
    ValueExpression(QVariant value, TokenType type)
        : value_(value)
        , type_(type){};

    QVariant execute(const ContextMap &context) override
    {
        if (this->type_ == TokenType::IDENTIFIER)
        {
            return context.value(this->value_.toString());
        }
        return this->value_;
    }

    TokenType type()
    {
        return this->type_;
    }

    QString debug() override
    {
        return QString("ValueExpression(%1, %2)")
            .arg(QString::number(this->type_), this->value_.toString());
    }

private:
    QVariant value_;
    TokenType type_;
};

class BinaryOperation : public Expression
{
public:
    BinaryOperation(BinaryOperator _op, Expression *_left, Expression *_right)
        : op(_op)
        , left(_left)
        , right(_right)
    {
    }

    BinaryOperator op;
    Expression *left;
    Expression *right;

    QVariant execute(const ContextMap &context) override
    {
        auto left = this->left->execute(context);
        auto right = this->right->execute(context);
        switch (this->op)
        {
            case Plus:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() + right.toInt();
                return 0;
            case Minus:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() - right.toInt();
                return 0;
            case Multiply:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() * right.toInt();
                return 0;
            case Divide:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() / right.toInt();
                return 0;
            case Modulus:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() % right.toInt();
                return 0;
            case Or:
                if (convertTypes(left, right, QMetaType::Bool))
                    return left.toBool() || right.toBool();
                return false;
            case And:
                if (convertTypes(left, right, QMetaType::Bool))
                    return left.toBool() && right.toBool();
                return false;
            case Equals:
                return left == right;
            case NotEquals:
                return left != right;
            case LessThan:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() < right.toInt();
                return false;
            case GreaterThan:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() > right.toInt();
                return false;
            case LessThanEqual:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() <= right.toInt();
                return false;
            case GreaterThanEqual:
                if (convertTypes(left, right, QMetaType::Int))
                    return left.toInt() >= right.toInt();
                return false;
            case Contains:
                if (left.type() == QVariant::Type::StringList &&
                    right.canConvert(QMetaType::QString))
                {
                    return left.toStringList().contains(right.toString(),
                                                        Qt::CaseInsensitive);
                }

                if (left.type() == QVariant::Type::Map &&
                    right.canConvert(QMetaType::QString))
                {
                    return left.toMap().contains(right.toString());
                }

                if (left.canConvert(QMetaType::QString) &&
                    right.canConvert(QMetaType::QString))
                {
                    return left.toString().contains(right.toString(),
                                                    Qt::CaseInsensitive);
                }

                return false;
        }
    }

    QString debug() override
    {
        return QString("BinaryOperation(%1, %2, %3)")
            .arg(this->left->debug(), QString::number(this->op),
                 this->right->debug());
    }
};

class UnaryOperation : public Expression
{
public:
    UnaryOperation(UnaryOperator _op, Expression *_right)
        : op(_op)
        , right(_right)
    {
    }

    UnaryOperator op;
    Expression *right;

    QVariant execute(const ContextMap &context) override
    {
        auto right = this->right->execute(context);
        switch (this->op)
        {
            case Not:
                if (right.canConvert<bool>())
                    return !right.toBool();
                return false;
        }
    }

    QString debug() override
    {
        return QString("UnaryOperation(%1, %2)")
            .arg(QString::number(this->op), this->right->debug());
    }
};

}  // namespace filterparser
