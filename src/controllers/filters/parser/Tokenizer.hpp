#pragma once

#include "controllers/filters/parser/Types.hpp"

#include <QMap>
#include <QRegularExpression>
#include <QString>

namespace filterparser {

struct IdentifierDescription {
    QString humanDescription;
    QMetaType::Type type;
};

static const QMap<QString, IdentifierDescription> validIdentifiersMap = {
    {"author.badges", {"author badges", QMetaType::QStringList}},
    {"author.color", {"author color", QMetaType::QColor}},
    {"author.name", {"author name", QMetaType::QString}},
    {"author.no_color", {"author has no color?", QMetaType::Bool}},
    {"author.subbed", {"author subscribed?", QMetaType::Bool}},
    {"author.sub_length", {"author sub length", QMetaType::Int}},
    {"channel.name", {"channel name", QMetaType::QString}},
    {"channel.watching", {"/watching channel?", QMetaType::Bool}},
    {"channel.live", {"channel live?", QMetaType::Bool}},
    {"flags.highlighted", {"highlighted?", QMetaType::Bool}},
    {"flags.points_redeemed", {"redeemed points?", QMetaType::Bool}},
    {"flags.sub_message", {"sub/resub message?", QMetaType::Bool}},
    {"flags.system_message", {"system message?", QMetaType::Bool}},
    {"flags.reward_message",
     {"channel point reward message?", QMetaType::Bool}},
    {"flags.first_message", {"first message?", QMetaType::Bool}},
    {"flags.elevated_message", {"elevated message?", QMetaType::Bool}},
    {"flags.cheer_message", {"cheer message?", QMetaType::Bool}},
    {"flags.whisper", {"whisper message?", QMetaType::Bool}},
    {"flags.reply", {"reply message?", QMetaType::Bool}},
    {"flags.automod", {"automod message?", QMetaType::Bool}},
    {"message.content", {"message text", QMetaType::QString}},
    {"message.length", {"message length", QMetaType::Int}}};

// clang-format off
static const QRegularExpression tokenRegex(
    QString("((r|ri)?\\\")((\\\\\")|[^\\\"])*\\\"|") +        // String/Regex literal
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
