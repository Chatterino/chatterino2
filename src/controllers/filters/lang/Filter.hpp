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
    QString debugString() const;

private:
    Filter(ExpressionPtr expression, Type returnType);

private:
    ExpressionPtr expression_;
    Type returnType_;
};

}  // namespace chatterino::filters
