#pragma once

#include "controllers/filters/parser/Types.hpp"

#include <QMetaType>
#include <QString>
#include <QVariant>

#include <memory>
#include <vector>

namespace filterparser {

class Expression
{
public:
    virtual ~Expression() = default;

    virtual QVariant execute(const ContextMap &) const;

    virtual PossibleType synthesizeType() const;

    virtual QString debug() const;
    virtual QString filterString() const;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using ExpressionList = std::vector<std::unique_ptr<Expression>>;

}  // namespace filterparser
