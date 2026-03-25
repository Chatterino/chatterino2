#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"

#include <memory>

namespace chatterino::filters {

std::unique_ptr<Expression> createValueAccessorExpression(const QString &name);

}  // namespace chatterino::filters
