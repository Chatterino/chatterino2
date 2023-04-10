#pragma once

#include "controllers/filters/lang/Types.hpp"

#include <QMap>
#include <QRegularExpression>
#include <QString>

namespace chatterino::filters {

static const QMap<QString, QString> validIdentifiersMap = {
    {"author.badges", "author badges"},
    {"author.color", "author color"},
    {"author.name", "author name"},
    {"author.no_color", "author has no color?"},
    {"author.subbed", "author subscribed?"},
    {"author.sub_length", "author sub length"},
    {"channel.name", "channel name"},
    {"channel.watching", "/watching channel?"},
    {"channel.live", "channel live?"},
    {"flags.highlighted", "highlighted?"},
    {"flags.points_redeemed", "redeemed points?"},
    {"flags.sub_message", "sub/resub message?"},
    {"flags.system_message", "system message?"},
    {"flags.reward_message", "channel point reward message?"},
    {"flags.first_message", "first message?"},
    {"flags.elevated_message", "elevated message?"},
    {"flags.cheer_message", "cheer message?"},
    {"flags.whisper", "whisper message?"},
    {"flags.reply", "reply message?"},
    {"flags.automod", "automod message?"},
    {"message.content", "message text"},
    {"message.length", "message length"},
    {"flags.webchat_detected", "Webchat detected"}};

// clang-format off
static const QRegularExpression tokenRegex(
    QString("((r|ri)?\\\")((\\\\\")|[^\\\"])*\\\"|") +        // String/Regex literal
    QString("[\\w\\.]+|") +                                   // Identifier or reserved keyword
    QString("(<=?|>=?|!=?|==|\\|\\||&&|\\+|-|\\*|\\/|%)+|") + // Operator
    QString("[\\(\\)]|") +                                    // Parentheses
    QString("[{},]")                                          // List
);
// clang-format on

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

QString tokenTypeToInfoString(TokenType type);

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
}  // namespace chatterino::filters
