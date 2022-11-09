#pragma once

#include <QRegularExpression>
#include <set>

#include <memory>

namespace chatterino {

struct Message;

}

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
    PossibleType(QMetaType::Type t)
    {
        this->types_.insert(t);
    }

    PossibleType(std::initializer_list<QMetaType::Type> t)
        : types_(t)
    {
        assert(!this->types_.empty());
    }

    QString string() const
    {
        if (this->types_.size() == 1)
        {
            return metaTypeToString(*this->types_.begin());
        }
        else
        {
            QStringList names;
            names.reserve(this->types_.size());
            for (QMetaType::Type t : this->types_)
            {
                names.push_back(metaTypeToString(t));
            }
            return "(" + names.join(" | ") + ")";
        }
    }

    bool operator==(QMetaType::Type t) const
    {
        return this->types_.count(t) != 0;
    }

    bool operator==(const PossibleType &p) const
    {
        // Check if there are any common types between the two sets
        auto i = this->types_.cbegin();
        auto j = p.types_.cbegin();
        while (i != this->types_.cend() && j != p.types_.cend())
        {
            if (*i == *j)
                return true;
            else if (*i < *j)
                ++i;
            else
                ++j;
        }
        return false;
    }

    bool operator!=(QMetaType::Type t) const
    {
        return this->types_.count(t) == 0;
    }

    bool operator!=(const PossibleType &p) const
    {
        return !this->operator==(p);
    }

private:
    std::set<QMetaType::Type> types_;
};

class Expression;
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

class Expression
{
public:
    virtual ~Expression() = default;

    virtual QVariant execute(const ContextMap &) const
    {
        return false;
    }

    virtual PossibleType returnType() const
    {
        return QMetaType::Bool;
    }

    virtual bool validateTypes(TypeValidator &validator) const
    {
        return true;
    }

    virtual QString debug() const
    {
        return "(false)";
    }

    virtual QString filterString() const
    {
        return "";
    }
};

using ExpressionPtr = std::unique_ptr<Expression>;

class ValueExpression : public Expression
{
public:
    ValueExpression(QVariant value, TokenType type);
    TokenType type();

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    QVariant value_;
    TokenType type_;
};

class RegexExpression : public Expression
{
public:
    RegexExpression(QString regex, bool caseInsensitive);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    QString regexString_;
    bool caseInsensitive_;
    QRegularExpression regex_;
};

using ExpressionList = std::vector<std::unique_ptr<Expression>>;

class ListExpression : public Expression
{
public:
    ListExpression(ExpressionList list);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    ExpressionList list_;
};

class BinaryOperation : public Expression
{
public:
    BinaryOperation(TokenType op, ExpressionPtr left, ExpressionPtr right);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    TokenType op_;
    ExpressionPtr left_;
    ExpressionPtr right_;
};

class UnaryOperation : public Expression
{
public:
    UnaryOperation(TokenType op, ExpressionPtr right);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    TokenType op_;
    ExpressionPtr right_;
};

}  // namespace filterparser
