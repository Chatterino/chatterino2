#pragma once

#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

static const QStringList validIdentifiers = {
    "message.content",             // String
    "message.length",              // Int
    "message.highlighted",         // Bool
    "author.name",                 // String
    "author.subscribed",           // Bool
    "author.subscription_length",  // Int
    "author.color",                // QColor
    "author.no_color",             // Bool
    "author.badges",               // String list
    "channel.name"                 // String
};

// clang-format off
static const QRegularExpression tokenRegex(
    QString("\\\"((\\\\\")|[^\\\"])*\\\"|") +                 // String literal
    QString("[\\w\\.]+|") +                                   // Identifier or reserved keyword
    QString("(<=?|>=?|!=?|==|\\|\\||&&|\\+|-|\\*|\\/|%)+|") + // Operator
    QString("[\\(\\)]")                                       // Parenthesis
);
// clang-format on

class Tokenizer
{
public:
    Tokenizer(const QString &text);

    bool hasNext() const;
    QString next();
    QString current() const;
    QString preview() const;
    TokenType nextTokenType() const;
    TokenType tokenType() const;

    bool nextTokenIsBinaryOp() const;
    bool nextTokenIsUnaryOp() const;
    bool nextTokenIsMathOp() const;

    void debug();
    const QStringList allTokens();

    static bool typeIsBinaryOp(TokenType token);
    static bool typeIsUnaryOp(TokenType token);
    static bool typeIsMathOp(TokenType token);

private:
    int i_ = 0;
    QStringList tokens_;
    QList<TokenType> tokenTypes_;

    TokenType tokenize(const QString &text);
};
}  // namespace filterparser
