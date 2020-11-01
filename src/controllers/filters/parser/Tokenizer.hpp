#pragma once

#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

static const QMap<QString, QString> validIdentifiersMap = {
    {"author.badges", "author badges"},
    {"author.color", "author color"},
    {"author.name", "author name"},
    {"author.no_color", "author has no color?"},
    {"author.subbed", "author subscribed?"},
    {"author.sub_length", "author sub length"},
    {"channel.name", "channel name"},
    {"channel.watching", "/watching channel?"},
    {"flags.highlighted", "highlighted?"},
    {"flags.points_redeemed", "redeemed points?"},
    {"flags.sub_message", "sub/resub message?"},
    {"flags.system_message", "system message?"},
    {"flags.whisper", "whisper message?"},
    {"message.content", "message text"},
    {"message.length", "message length"}};

// clang-format off
static const QRegularExpression tokenRegex(
    QString("\\\"((\\\\\")|[^\\\"])*\\\"|") +                 // String literal
    QString("[\\w\\.]+|") +                                   // Identifier or reserved keyword
    QString("(<=?|>=?|!=?|==|\\|\\||&&|\\+|-|\\*|\\/|%)+|") + // Operator
    QString("[\\(\\)]|") +                                    // Parentheses
    QString("[{},]")                                          // List
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
