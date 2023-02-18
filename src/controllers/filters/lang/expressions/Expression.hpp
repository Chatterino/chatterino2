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

    virtual QVariant execute(const ContextMap &context) const;
    virtual PossibleType synthesizeType(const TypingContext &context) const;
    virtual QString debug(const TypingContext &context) const;
    virtual QString filterString() const;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using ExpressionList = std::vector<std::unique_ptr<Expression>>;

}  // namespace chatterino::filters
