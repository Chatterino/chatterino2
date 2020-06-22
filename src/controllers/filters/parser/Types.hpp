#pragma once

#import "messages/Message.hpp"

namespace filterparser {

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

enum BinaryOperator {
    And = 1,
    Or = 2,

    Plus = 101,
    Minus = 102,
    Multiply = 103,
    Divide = 104,
    Modulus = 105,

    Equals = 11,
    NotEquals = 12,
    LessThan = 13,
    GreaterThan = 14,
    LessThanEqual = 15,
    GreaterThanEqual = 16,
    Contains = 17
};

enum UnaryOperator { Not = 51 };

namespace {
    bool convertTypes(QVariant &a, QVariant &b, int type)
    {
        return a.convert(type) && b.convert(type);
    }

    QString tokenTypeToString(TokenType type)
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
                return "";
            case AND:
                return "&&";
            case OR:
                return "||";
            case LP:
                return "(";
            case RP:
                return ")";
            case PLUS:
                return "+";
            case MINUS:
                return "-";
            case MULTIPLY:
                return "*";
            case DIVIDE:
                return "/";
            case MOD:
                return "%";
            case EQ:
                return "==";
            case NEQ:
                return "!=";
            case LT:
                return "<";
            case GT:
                return ">";
            case LTE:
                return "<=";
            case GTE:
                return ">=";
            case CONTAINS:
                return "contains";
            case NOT:
                return "!";
            case STRING:
                return "<string>";
            case INT:
                return "<int>";
            case IDENTIFIER:
                return "<identifier>";
        }
    }

    QString binaryOperatorToString(BinaryOperator op)
    {
        return tokenTypeToString(TokenType(op));
    }

    QString unaryOperatorToString(UnaryOperator op)
    {
        return tokenTypeToString(TokenType(op));
    }
}  // namespace

using MessagePtr = std::shared_ptr<const chatterino::Message>;
using ContextMap = QMap<QString, QVariant>;

class Expression
{
public:
    virtual QVariant execute(const ContextMap &)
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
            .arg(tokenTypeToString(this->type_), this->value_.toString());
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
            .arg(this->left->debug(), binaryOperatorToString(this->op),
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
            .arg(unaryOperatorToString(this->op), this->right->debug());
    }
};

}  // namespace filterparser
