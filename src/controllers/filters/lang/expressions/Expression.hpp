#pragma once

#include "controllers/filters/lang/Tokenizer.hpp"
#include "controllers/filters/lang/Types.hpp"

#include <QString>
#include <QVariant>

#include <memory>
#include <vector>

namespace chatterino::filters {

class Expression
{
public:
    virtual ~Expression() = default;

    virtual QVariant execute(const ContextMap &context) const = 0;
    virtual PossibleType synthesizeType(const TypingContext &context) const = 0;
    virtual QString debug(const TypingContext &context) const = 0;
    virtual QString filterString() const = 0;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using ExpressionList = std::vector<std::unique_ptr<Expression>>;

}  // namespace chatterino::filters
