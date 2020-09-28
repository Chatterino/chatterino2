#pragma once

#include "controllers/filters/parser/Tokenizer.hpp"
#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

ContextMap buildContextMap(const MessagePtr &m);

class FilterParser
{
public:
    FilterParser(const QString &text);
    bool execute(const MessagePtr &message) const;
    bool execute(const ContextMap &context) const;
    bool valid() const;

    const QStringList &errors() const;
    const QString debugString() const;
    const QString filterString() const;

private:
    ExpressionPtr parseExpression(bool top = false);
    ExpressionPtr parseAnd();
    ExpressionPtr parseUnary();
    ExpressionPtr parseParentheses();
    ExpressionPtr parseCondition();
    ExpressionPtr parseValue();

    void errorLog(const QString &text, bool expand = false);

    QString text_;
    Tokenizer tokenizer_;
    ExpressionPtr builtExpression_;

    QStringList parseLog_;
    bool valid_ = true;
};
}  // namespace filterparser
