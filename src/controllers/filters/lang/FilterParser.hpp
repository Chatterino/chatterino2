#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Tokenizer.hpp"
#include "controllers/filters/lang/Types.hpp"

namespace chatterino::filters {

class FilterParser
{
public:
    /**
     * Take input text & attempt to parse it into a filter
     **/
    FilterParser(const QString &text);

    bool valid() const;
    Type returnType() const;
    ExpressionPtr release();

    const QStringList &errors() const;
    const QString debugString() const;

private:
    ExpressionPtr parseExpression(bool top = false);
    ExpressionPtr parseAnd();
    ExpressionPtr parseUnary();
    ExpressionPtr parseParentheses();
    ExpressionPtr parseCondition();
    ExpressionPtr parseValue();
    ExpressionPtr parseList();

    void errorLog(const QString &text, bool expand = false);

    QStringList parseLog_;
    bool valid_ = true;

    QString text_;
    Tokenizer tokenizer_;
    ExpressionPtr builtExpression_;
    Type returnType_ = Type::Bool;
};

}  // namespace chatterino::filters
