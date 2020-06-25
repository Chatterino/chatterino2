#pragma once

#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

static const QStringList validIdentifiers = {
    "author.badges",          // String list
    "author.color",           // QColor
    "author.name",            // String
    "author.no_color",        // Bool
    "author.subbed",          // Bool
    "author.sub_length",      // Int
    "channel.name",           // String
    "channel.watching",       // Bool
    "flags.highlighted",      // Bool
    "flags.points_redeemed",  // Bool
    "flags.sub_message",      // Bool
    "flags.system_message",   // Bool
    "flags.whisper",          // Bool
    "message.content",        // String
    "message.length"          // Int
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

    bool nextTokenIsOp() const;
    bool nextTokenIsBinaryOp() const;
    bool nextTokenIsUnaryOp() const;
    bool nextTokenIsMathOp() const;

    void debug();
    const QStringList allTokens();

    static bool typeIsOp(TokenType token);
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
