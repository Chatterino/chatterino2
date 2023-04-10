#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Types.hpp"

#include <QString>

#include <memory>
#include <variant>

namespace chatterino {

class Channel;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;

}  // namespace chatterino

namespace chatterino::filters {

// MESSAGE_TYPING_CONTEXT maps filter variables to their expected type at evaluation.
// For example, flags.highlighted is a boolean variable, so it is marked as Type::Bool
// below. These variable types will be used to check whether a filter "makes sense",
// i.e. if all the variables and operators being used have compatible types.
static const QMap<QString, Type> MESSAGE_TYPING_CONTEXT = {
    {"author.badges", Type::StringList},
    {"author.color", Type::Color},
    {"author.name", Type::String},
    {"author.no_color", Type::Bool},
    {"author.subbed", Type::Bool},
    {"author.sub_length", Type::Int},
    {"channel.name", Type::String},
    {"channel.watching", Type::Bool},
    {"channel.live", Type::Bool},
    {"flags.highlighted", Type::Bool},
    {"flags.points_redeemed", Type::Bool},
    {"flags.sub_message", Type::Bool},
    {"flags.system_message", Type::Bool},
    {"flags.reward_message", Type::Bool},
    {"flags.first_message", Type::Bool},
    {"flags.elevated_message", Type::Bool},
    {"flags.cheer_message", Type::Bool},
    {"flags.whisper", Type::Bool},
    {"flags.reply", Type::Bool},
    {"flags.automod", Type::Bool},
    {"message.content", Type::String},
    {"message.length", Type::Int},

    // dankerino
    {"flags.webchat_detected", Type::Bool},
};

ContextMap buildContextMap(const MessagePtr &m, chatterino::Channel *channel);

class Filter;
struct FilterError {
    QString message;
};

using FilterResult = std::variant<Filter, FilterError>;

class Filter
{
public:
    static FilterResult fromString(const QString &str);

    Type returnType() const;
    QVariant execute(const ContextMap &context) const;

    QString filterString() const;
    QString debugString(const TypingContext &context) const;

private:
    Filter(ExpressionPtr expression, Type returnType);

    ExpressionPtr expression_;
    Type returnType_;
};

}  // namespace chatterino::filters
