#pragma once

#include "controllers/filters/parser/Types.hpp"

#include <QMap>
#include <QRegularExpression>
#include <QString>

namespace chatterino::filters {

struct IdentifierDescription {
    QString humanDescription;
    Type type;
};

using T = Type;

static const QMap<QString, IdentifierDescription> validIdentifiersMap = {
    {"author.badges", {"author badges", T::List}},
    {"author.color", {"author color", T::Color}},
    {"author.name", {"author name", T::String}},
    {"author.no_color", {"author has no color?", T::Bool}},
    {"author.subbed", {"author subscribed?", T::Bool}},
    {"author.sub_length", {"author sub length", T::Int}},
    {"channel.name", {"channel name", T::String}},
    {"channel.watching", {"/watching channel?", T::Bool}},
    {"channel.live", {"channel live?", T::Bool}},
    {"flags.highlighted", {"highlighted?", T::Bool}},
    {"flags.points_redeemed", {"redeemed points?", T::Bool}},
    {"flags.sub_message", {"sub/resub message?", T::Bool}},
    {"flags.system_message", {"system message?", T::Bool}},
    {"flags.reward_message", {"channel point reward message?", T::Bool}},
    {"flags.first_message", {"first message?", T::Bool}},
    {"flags.elevated_message", {"elevated message?", T::Bool}},
    {"flags.cheer_message", {"cheer message?", T::Bool}},
    {"flags.whisper", {"whisper message?", T::Bool}},
    {"flags.reply", {"reply message?", T::Bool}},
    {"flags.automod", {"automod message?", T::Bool}},
    {"message.content", {"message text", T::String}},
    {"message.length", {"message length", T::Int}}};

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
