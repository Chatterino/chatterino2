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
extern const QMap<QString, Type> MESSAGE_TYPING_CONTEXT;

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
