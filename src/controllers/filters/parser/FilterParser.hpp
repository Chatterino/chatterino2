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
    Expression *parseExpression(bool top = false);
    Expression *parseAnd();
    Expression *parseUnary();
    Expression *parseParenthesis();
    Expression *parseCondition();
    Expression *parseValue();

    void errorLog(const QString &text, bool expand = false);

    QString text_;
    Tokenizer tokenizer_;
    Expression *builtExpression_;

    QStringList parseLog_;
    bool valid_ = true;
};
}  // namespace filterparser
